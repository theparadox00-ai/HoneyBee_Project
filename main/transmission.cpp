#include "transmission.h"

SMTPSession smtp;

void smtpCallback(SMTP_Status status) {
    Serial.println(status.info());
}

bool Send_All_Data_Email(int soc, float voltage) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[TX] ERR: WiFi not connected");
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

    SMTP_Attachment* att_lc  = nullptr;
    SMTP_Attachment* att_sht = nullptr;

    if (SD.exists(LC_TX_PATH)) {
        Serial.printf("[TX] Attaching CSV: %s\n", LC_TX_PATH);
        att_lc = new SMTP_Attachment();
        att_lc->descr.filename    = "LoadCell_TX.csv";
        att_lc->descr.mime        = "text/csv";
        att_lc->file.path         = LC_TX_PATH;
        att_lc->file.storage_type = esp_mail_file_storage_type_sd;
        message.addAttachment(*att_lc);
    } else {
        Serial.println("[TX] WARN: LC CSV not found");
    }

    if (SD.exists(SHT_TX_PATH)) {
        Serial.printf("[TX] Attaching CSV: %s\n", SHT_TX_PATH);
        att_sht = new SMTP_Attachment();
        att_sht->descr.filename    = "SHT45_TX.csv";
        att_sht->descr.mime        = "text/csv";
        att_sht->file.path         = SHT_TX_PATH;
        att_sht->file.storage_type = esp_mail_file_storage_type_sd;
        message.addAttachment(*att_sht);
    } else {
        Serial.println("[TX] WARN: SHT CSV not found");
    }

    const int MAX_WAV_ATT = 10;
    SMTP_Attachment* wav_atts[MAX_WAV_ATT];
    char wav_paths[MAX_WAV_ATT][80];
    char wav_names[MAX_WAV_ATT][40];
    int  wav_count = 0;

    Serial.printf("[TX] Opening dir: %s\n", DIR_AUDIO_TX);
    File audioDir = SD.open(DIR_AUDIO_TX);
    if (!audioDir) {
        Serial.println("[TX] WARN: Could not open TX audio dir");
    } else if (!audioDir.isDirectory()) {
        Serial.println("[TX] WARN: TX audio path is not a directory");
        audioDir.close();
    } else {
        File entry = audioDir.openNextFile();
        while (entry && wav_count < MAX_WAV_ATT) {
            if (!entry.isDirectory()) {
                const char* rawName = entry.name();
                Serial.printf("[TX] entry.name() raw = '%s'\n", rawName);
                if (rawName[0] == '/') {
                    strncpy(wav_paths[wav_count], rawName, sizeof(wav_paths[0]) - 1);
                } else {
                    snprintf(wav_paths[wav_count], sizeof(wav_paths[0]),
                             "%s/%s", DIR_AUDIO_TX, rawName);
                }
                wav_paths[wav_count][sizeof(wav_paths[0]) - 1] = '\0';

                const char* slash = strrchr(wav_paths[wav_count], '/');
                strncpy(wav_names[wav_count],
                        slash ? slash + 1 : wav_paths[wav_count],
                        sizeof(wav_names[0]) - 1);
                wav_names[wav_count][sizeof(wav_names[0]) - 1] = '\0';

                entry.close();

                if (!SD.exists(wav_paths[wav_count])) {
                    Serial.printf("[TX] WARN: WAV not found at '%s' – skipping\n",
                                  wav_paths[wav_count]);
                } else {
                    Serial.printf("[TX] Attaching WAV: '%s' as '%s'\n",
                                  wav_paths[wav_count], wav_names[wav_count]);
                    wav_atts[wav_count] = new SMTP_Attachment();
                    wav_atts[wav_count]->descr.filename    = wav_names[wav_count];
                    wav_atts[wav_count]->descr.mime        = "audio/wav";
                    wav_atts[wav_count]->file.path         = wav_paths[wav_count];
                    wav_atts[wav_count]->file.storage_type = esp_mail_file_storage_type_sd;
                    message.addAttachment(*wav_atts[wav_count]);
                    wav_count++;
                }
            } else {
                entry.close();
            }
            entry = audioDir.openNextFile();
        }
        audioDir.close();
    }

    Serial.printf("[TX] Total WAV attachments: %d\n", wav_count);

    bool success = false;
    if (!smtp.connect(&session)) {
        Serial.println("[TX] ERR: SMTP connect failed");
    } else if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println("[TX] ERR: send failed – " + smtp.errorReason());
    } else {
        Serial.println("[TX] Email sent successfully!");
        success = true;
    }
    smtp.closeSession();

    if (att_lc)  delete att_lc;
    if (att_sht) delete att_sht;
    for (int i = 0; i < wav_count; i++) delete wav_atts[i];

    if (success) clearTxFiles();
    return success;
}

bool Send_NightSleep_Email(int soc, float voltage) {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("[TX] Night email – connecting WiFi");
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 25000) {
        delay(500); Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[TX] Night email – WiFi failed, skipping.");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
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
    message.subject      = "Beehive – Entering Night Sleep";
    message.addRecipient("Keeper", RECIPIENT_EMAIL);

    String body  = "The beehive monitor is entering night sleep mode.\n\n";
           body += "Battery: " + String(soc) + "% / " + String(voltage, 2) + " V\n";
           body += "Will wake at next sunrise.\n";
    message.text.content = body;

    bool success = false;
    if (!smtp.connect(&session)) {
        Serial.println("[TX] Night email ERR: SMTP connect failed");
    } else if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println("[TX] Night email ERR: " + smtp.errorReason());
    } else {
        Serial.println("[TX] Night sleep email sent OK.");
        success = true;
    }
    smtp.closeSession();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return success;
}

bool Send_LowBattery_Email(int soc, float voltage) {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("[TX] Low-batt email – connecting WiFi");
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 25000) {
        delay(500); Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[TX] Low-batt email – WiFi failed, skipping.");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
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
    message.subject      = "Beehive Monitor – LOW BATTERY";
    message.addRecipient("Keeper", RECIPIENT_EMAIL);

    String body  = "LOW BATTERY ALERT \n\n";
           body += "The beehive monitor battery is critically low.\n";
           body += "Battery: " + String(soc) + "% / " + String(voltage, 3) + " V\n";
           body += "Threshold: 3.600 V\n\n";
           body += "The system has entered permanent deep sleep.\n";
           body += "Please recharge the battery to resume monitoring.\n";
    message.text.content = body;

    bool success = false;
    if (!smtp.connect(&session)) {
        Serial.println("[TX] Low-batt email ERR: SMTP connect failed");
    } else if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println("[TX] Low-batt email ERR: " + smtp.errorReason());
    } else {
        Serial.println("[TX] Low-battery email sent OK.");
        success = true;
    }
    smtp.closeSession();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return success;
}

bool Send_WakeupEmail(int soc, float voltage) {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("[TX] Wake-up email – connecting WiFi");
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 25000) {
        delay(500); Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[TX] Wake-up email – WiFi failed, skipping.");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
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
    message.subject      = "Beehive Monitor – Good Morning! (Waking Up)";
    message.addRecipient("Keeper", RECIPIENT_EMAIL);

    String body  = "Good morning!\n\n";
           body += "The beehive monitor has woken from its nightly sleep\n";
           body += "and is resuming daytime monitoring.\n\n";
           body += "Battery: " + String(soc) + "% / " + String(voltage, 2) + " V\n";
           body += "Daytime monitoring is now active.\n";
    message.text.content = body;

    bool success = false;
    if (!smtp.connect(&session)) {
        Serial.println("[TX] Wake-up email ERR: SMTP connect failed");
    } else if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println("[TX] Wake-up email ERR: " + smtp.errorReason());
    } else {
        Serial.println("[TX] Wake-up email sent OK.");
        success = true;
    }
    smtp.closeSession();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return success;
}
