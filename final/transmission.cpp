#include "transmission.h"

SMTPSession smtp;

void smtpCallback(SMTP_Status status) {
    Serial.println(status.info());
}

bool Send_All_Data_Email(int soc, float voltage) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("ERR: WiFi not connected – cannot send email");
        return false;
    }

    smtp.debug(1);
    smtp.callback(smtpCallback);

    ESP_Mail_Session session;
    session.server.host_name  = SMTP_HOST;
    session.server.port       = SMTP_PORT;
    session.login.email       = AUTHOR_EMAIL;
    session.login.password    = AUTHOR_PASSWORD;
    session.login.user_domain = "gmail.com";

    SMTP_Message message;
    message.sender.name  = "Beehive Monitor";
    message.sender.email = AUTHOR_EMAIL;
    message.subject      = "Beehive Sensor + Audio Data";
    message.addRecipient("Keeper", RECIPIENT_EMAIL);

    String body  = "Battery: " + String(soc) + "% / " + String(voltage, 2) + " V\n";
           body += "Attached: LoadCell CSV, SHT45 CSV, and WAV audio recordings.\n";
    message.text.content = body;

    // ── Attach CSV TX buffers ────────────────────────────────
    SMTP_Attachment* att_lc  = nullptr;
    SMTP_Attachment* att_sht = nullptr;

    if (SD.exists(LC_TX_PATH)) {
        att_lc = new SMTP_Attachment();
        att_lc->descr.filename    = "LoadCell_TX.csv";
        att_lc->descr.mime        = "text/csv";
        att_lc->file.path         = LC_TX_PATH;
        att_lc->file.storage_type = esp_mail_file_storage_type_sd;
        message.addAttachment(*att_lc);
    }
    if (SD.exists(SHT_TX_PATH)) {
        att_sht = new SMTP_Attachment();
        att_sht->descr.filename    = "SHT45_TX.csv";
        att_sht->descr.mime        = "text/csv";
        att_sht->file.path         = SHT_TX_PATH;
        att_sht->file.storage_type = esp_mail_file_storage_type_sd;
        message.addAttachment(*att_sht);
    }

    // ── Attach all WAVs from /Transmit/Audio/ ────────────────
    // Collect up to 10 WAV attachments (Gmail ~25 MB limit)
    const int MAX_WAV_ATT = 10;
    SMTP_Attachment* wav_atts[MAX_WAV_ATT];
    char wav_paths[MAX_WAV_ATT][64];
    char wav_names[MAX_WAV_ATT][32];
    int  wav_count = 0;

    File audioDir = SD.open(DIR_AUDIO_TX);
    if (audioDir) {
        File entry = audioDir.openNextFile();
        while (entry && wav_count < MAX_WAV_ATT) {
            if (!entry.isDirectory()) {
                snprintf(wav_paths[wav_count], sizeof(wav_paths[0]),
                         "%s/%s", DIR_AUDIO_TX, entry.name());
                snprintf(wav_names[wav_count], sizeof(wav_names[0]),
                         "%s", entry.name());
                entry.close();

                wav_atts[wav_count] = new SMTP_Attachment();
                wav_atts[wav_count]->descr.filename    = wav_names[wav_count];
                wav_atts[wav_count]->descr.mime        = "audio/wav";
                wav_atts[wav_count]->file.path         = wav_paths[wav_count];
                wav_atts[wav_count]->file.storage_type = esp_mail_file_storage_type_sd;
                message.addAttachment(*wav_atts[wav_count]);
                wav_count++;
            } else {
                entry.close();
            }
            entry = audioDir.openNextFile();
        }
        audioDir.close();
    }
    Serial.printf("[TX] Attaching %d WAV file(s)\n", wav_count);

    // ── Connect and send ─────────────────────────────────────
    bool success = false;
    if (!smtp.connect(&session)) {
        Serial.println("ERR: SMTP connect failed");
    } else if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println("ERR: Email failed – " + smtp.errorReason());
    } else {
        Serial.println("Email sent successfully!");
        success = true;
    }

    smtp.closeSession();

    // Cleanup heap attachments
    if (att_lc)  delete att_lc;
    if (att_sht) delete att_sht;
    for (int i = 0; i < wav_count; i++) delete wav_atts[i];

    // Clear TX buffers ONLY on success
    if (success) clearTxFiles();

    return success;
}
