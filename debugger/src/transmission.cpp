#include "transmission.h"

const char* SMTP_HOST       = "smtp.gmail.com";
const int   SMTP_PORT       = 465;
const char* AUTHOR_EMAIL    = "suryadiya04@gmail.com";
const char* AUTHOR_PASSWORD = "**** **** **** ****"; 
const char* RECIPIENT_EMAIL = "suryadiya04@gmail.com";

SMTPSession smtp;

void smtpCallback(SMTP_Status status) {
    Serial.println(status.info());
}

bool Send_All_Data_Email(float battery_voltage , float battery_percentage) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: WiFi not connected!");
        return false;
    }

    smtp.debug(1); 
    smtp.callback(smtpCallback);

    ESP_Mail_Session session;
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;
    session.login.user_domain = "";

    SMTP_Message message;
    message.sender.name = "ESP32 Data";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "ESP32 Sensor Dataset";
    message.addRecipient("User", RECIPIENT_EMAIL);

    String emailBody = "";
    emailBody.reserve(64);
    emailBody.printf("Data Logs Attached.\nBattery Status\n");
    emailBody.printf("Battery Voltage: %.1fV\tBattery Percentage: %.1f%%\n",battery_voltage, battery_percentage);
    
    SMTP_Attachment *att1 = nullptr;
    SMTP_Attachment *att2 = nullptr;

    String lcPath = String(DIR_LOADCELL) + "/data.csv";
    if (SD.exists(lcPath)) {
        att1 = new SMTP_Attachment;
        att1->descr.filename = "LoadCell_Data.csv";
        att1->descr.mime = "text/csv"; 
        att1->file.path = lcPath.c_str();
        att1->file.storage_type = esp_mail_file_storage_type_sd; 
        message.addAttachment(*att1);
    } else {
        Serial.println("Warning: LC file missing");
    }

    String shtPath = String(DIR_SHT) + "/data.csv";
    if (SD.exists(shtPath)) {
        att2 = new SMTP_Attachment;
        att2->descr.filename = "SHT45_Data.csv";
        att2->descr.mime = "text/csv";
        att2->file.path = shtPath.c_str();
        att2->file.storage_type = esp_mail_file_storage_type_sd;
        message.addAttachment(*att2);
    } else {
        Serial.println("Warning: SHT file missing");
    }

    if (!smtp.connect(&session)) {
        Serial.println("SMTP Connect Failed");
        return false;
    }

    if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println("Error sending Email: " + smtp.errorReason());
        return false;
    }
    
    if (att1) delete att1;
    if (att2) delete att2;

    Serial.println("Email Sent Successfully!");
    return true;
}


