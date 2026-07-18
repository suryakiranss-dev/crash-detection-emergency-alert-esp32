// secrets_template.h
// ------------------------------------------------------------
// HOW TO USE:
// 1. Copy this file and rename the copy to "secrets.h" in this same folder.
// 2. Fill in your real WiFi credentials, Firebase API key, and DB URL below.
// 3. NEVER commit secrets.h to GitHub — it's already excluded via .gitignore.
// ------------------------------------------------------------

#ifndef SECRETS_H
#define SECRETS_H

// WiFi credentials
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

// Firebase project credentials
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_PROJECT-default-rtdb.firebaseio.com"

// Vehicle identifier used in Firebase paths
String vehicleID = "YOUR_VEHICLE_ID";

#endif
