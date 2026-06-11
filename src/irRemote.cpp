#include <IRrecv.h>
#include <IRsend.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>
#include <Preferences.h>
#include "irRemote.h"

// ========== PIN ==========
#define IR_RECV_PIN  15
#define IR_SEND_PIN  4

// ========== PARAMETER ==========
const uint16_t kCaptureBufferSize   = 1024;
const uint8_t  kTimeout             = 50;
const uint16_t kMinUnknownSize      = 12;
const uint8_t  kTolerancePercentage = kTolerance;

struct IrRemoteHandler {
  IRrecv irrecv;
  IRac ac;
  Preferences prefs;
  decode_results results;

  stdAc::state_t acState;
  stdAc::state_t acPrevState;
  bool prevStateValid;

  uint8_t baseState[kStateSizeMax];
  uint16_t stateLen;
  bool scanMode;
  String protoName;

  IrRemoteHandler(uint16_t recvPin, uint16_t sendPin) //constructor
    : irrecv(recvPin, kCaptureBufferSize, kTimeout, true),
      ac(sendPin),
      prevStateValid(false),
      stateLen(0),
      scanMode(false),
      protoName("UNKNOWN")
  {
    memset(baseState, 0, sizeof(baseState));
  }

  void init();
  void loop();
  void processCommand(String cmd);
  void startScan();
  void sendSignal();

private:
  void processScanResult();
  void setDefaultAcState();
  void resetConfig();
  void saveToFlash();
  void loadFromFlash();
  void printStatus();
  void printHelp();
};

//global object 
static IrRemoteHandler irRemote(IR_RECV_PIN, IR_SEND_PIN);

//public func 
void irRemoteInit() { irRemote.init();}
void irRemoteLoop() { irRemote.loop();}
void irRemoteProcessCommand(String cmd) { irRemote.processCommand(cmd);}
bool isScanMode() { return irRemote.scanMode;}
void irRemoteScan() { irRemote.startScan();}
void irRemoteSendSignal() { irRemote.sendSignal();}

stdAc::state_t* irRemotegetAcState() { return &irRemote.acState;}
const String* irRemotegetProtoName() { return &irRemote.protoName;}

//struct method 
void IrRemoteHandler::init() {
//   irrecv.setUnknownThreshold(kMinUnknownSize);
  irrecv.setTolerance(kTolerancePercentage);
  irrecv.enableIRIn();

  setDefaultAcState();
  loadFromFlash();

  if (stateLen == 0) {
    scanMode = true;
    Serial.println("no irData");
  } 
  else {
    scanMode = false;
    Serial.print("protocol :");
    Serial.println(protoName);
    Serial.print("model :");
    Serial.println(acState.model);
    printHelp();
  }

  printStatus();
}


#if 0 //original
void IrRemoteHandler::loop() {
  if (irrecv.decode(&results)) {
    if (scanMode) {
      processScanResult();
    } else {
      if (!results.repeat && hasACState(results.decode_type)) {
        if (results.decode_type == acState.protocol) {
          Serial.println("\n>>> Remote asli, sync state...");

          stdAc::state_t s;
          if (IRAcUtils::decodeToState(&results, &s, nullptr)) {
            acState.power    = 
            s.power;
            acState.degrees  = s.degrees;
            acState.mode     = s.mode;
            acState.fanspeed = s.fanspeed;
            acState.swingv   = s.swingv;
            acState.swingh   = s.swingh;

            Serial.println(IRAcUtils::resultAcToString(&results));
          }
        }
      }
    }

    irrecv.resume();
  }
}
#endif


void IrRemoteHandler::startScan() {
    if (irrecv.decode(&results)){
        processScanResult();    
    }

    irrecv.resume(); 
}

///////////////////////////////////////////////////////// ///////////////////////////////////////////


void IrRemoteHandler::sendSignal() {
    if (acState.protocol == decode_type_t::UNKNOWN || stateLen == 0) {
    Serial.println("err: ss1");
    return;
  }
  if (!ac.isProtocolSupported(acState.protocol)) {
    Serial.println("❌ Protocol tidak didukung IRac: " + protoName);
    return;
  }

  Serial.print("Mengirim ["); Serial.print(protoName);
  Serial.print("] P="); Serial.print(acState.power ? "ON" : "OFF");
  Serial.print(" T="); Serial.print(acState.degrees);
  Serial.print(" M="); Serial.print(IRac::opmodeToString(acState.mode));
  Serial.print(" F="); Serial.print(IRac::fanspeedToString(acState.fanspeed));
  Serial.print(" SV="); Serial.println(IRac::swingvToString(acState.swingv));

  // Kirim - IRac otomatis handle checksum & format semua merk
  stdAc::state_t* prev = prevStateValid ? &acPrevState : nullptr;
  ac.sendAc(acState, prev);

  acPrevState    = acState;
  prevStateValid = true;

  Serial.println("✓ Terkirim!");
  saveToFlash();
  printStatus();
}

// ========== PROSES SCAN ==========
void IrRemoteHandler::processScanResult() {
  if (results.repeat) { Serial.println("⚠ Repeat, coba lagi..."); return; }
  if (results.rawlen < 10) { Serial.println("⚠ Sinyal pendek..."); return; }

  decode_type_t proto = results.decode_type;

#if 0 //makan flash mayan gede 
  // Output dump
  uint32_t now = millis();
  Serial.printf("\nTimestamp : %06u.%03u\n", now / 1000, now % 1000);
  Serial.println("Library   : v" _IRREMOTEESP8266_VERSION_STR "\n");
  if (results.overflow) Serial.println("⚠ Buffer penuh!");
  Serial.print(resultToHumanReadableBasic(&results));
  String desc = IRAcUtils::resultAcToString(&results);
  if (desc.length()) Serial.println("Mesg Desc.: " + desc);
  Serial.println(resultToSourceCode(&results));
  Serial.println();
#endif
  if (!hasACState(proto)) {
    Serial.println("⚠ Bukan AC protocol! Coba lagi...");
    return;
  }

  if (!ac.isProtocolSupported(proto)) {
    Serial.println("⚠ Protocol tidak didukung IRac: " + typeToString(proto, false));
    return;
  }

  // Simpan base state
  stateLen = results.bits / 8;
  memcpy(baseState, results.state, stateLen);
  protoName = typeToString(proto, false);

  // Decode langsung ke stdAc::state_t
  if (IRAcUtils::decodeToState(&results, &acState, nullptr)) {
    Serial.println("✓ State decoded!");
  } else {
    // Fallback: set protocol & model saja
    acState.protocol = proto;
    acState.model    = -1;
  }

  // Set default yang mungkin tidak ter-decode
  acState.light  = false;
  acState.clean  = false;
  acState.beep   = false;
  acState.sleep  = -1;
  acState.clock  = -1;

  prevStateValid = false; // reset prev state
  scanMode       = false;
  saveToFlash();

  Serial.println("✓ Scan selesai!");
  Serial.println("✓ Protocol : " + protoName);
  Serial.println("✓ Model    : " + String(acState.model));
  printHelp();
  printStatus();
  ESP.restart(); //restart after get right remote 

}

// ========== PROSES COMMAND ==========
void IrRemoteHandler::processCommand(String cmd) {
    cmd.trim();
    String c = cmd;
    c.toLowerCase();

  if (c == "scan") {
    scanMode = true;
    Serial.println("================================");
    Serial.println("Mode SCAN aktif!");
    Serial.println("Tekan sembarang tombol remote AC");
    Serial.println("================================");

  } else if (c == "power on") {
    acState.power = true;  sendSignal();
  } else if (c == "power off") {
    acState.power = false; sendSignal();

  } else if (c.startsWith("temp ")) {
    float t = c.substring(5).toFloat();
    if (t >= 16 && t <= 30) { acState.degrees = t; sendSignal(); }
    else Serial.println("Suhu harus 16-30!");

  // Mode - pakai stdAc enum langsung
  } else if (c == "mode cool") { acState.mode = stdAc::opmode_t::kCool; sendSignal();
  } else if (c == "mode heat") { acState.mode = stdAc::opmode_t::kHeat; sendSignal();
  } else if (c == "mode dry")  { acState.mode = stdAc::opmode_t::kDry;  sendSignal();
  } else if (c == "mode fan")  { acState.mode = stdAc::opmode_t::kFan;  sendSignal();
  } else if (c == "mode auto") { acState.mode = stdAc::opmode_t::kAuto; sendSignal();

  // Fan - pakai stdAc enum langsung
  } else if (c == "fan auto") { acState.fanspeed = stdAc::fanspeed_t::kAuto;   sendSignal();
  } else if (c == "fan min")  { acState.fanspeed = stdAc::fanspeed_t::kMin;    sendSignal();
  } else if (c == "fan low")  { acState.fanspeed = stdAc::fanspeed_t::kLow;    sendSignal();
  } else if (c == "fan med")  { acState.fanspeed = stdAc::fanspeed_t::kMedium; sendSignal();
  } else if (c == "fan high") { acState.fanspeed = stdAc::fanspeed_t::kHigh;   sendSignal();
  } else if (c == "fan max")  { acState.fanspeed = stdAc::fanspeed_t::kMax;    sendSignal();

  // Swing Vertikal
  } else if (c == "swingv on")  {
    acState.swingv = stdAc::swingv_t::kAuto; sendSignal();
  } else if (c == "swingv off") {
    acState.swingv = stdAc::swingv_t::kOff;  sendSignal();

  // Swing Horizontal
  } else if (c == "swingh on")  {
    acState.swingh = stdAc::swingh_t::kAuto; sendSignal();
  } else if (c == "swingh off") {
    acState.swingh = stdAc::swingh_t::kOff;  sendSignal();

  // Turbo & Econo
  } else if (c == "turbo on")  { acState.turbo = true;  sendSignal();
  } else if (c == "turbo off") { acState.turbo = false; sendSignal();
  } else if (c == "econo on")  { acState.econo = true;  sendSignal();
  } else if (c == "econo off") { acState.econo = false; sendSignal();

  } else if (c == "send")   { sendSignal();
  } else if (c == "status") { printStatus();
  } else if (c == "help")   { printHelp();

  } else if (c == "reset") {
    resetConfig();
    Serial.println("✓ Reset! Ketik 'scan'");

  } else {
    Serial.println("Command tidak dikenal. Ketik 'help'");
  }
}


void IrRemoteHandler::setDefaultAcState() {
  acState.protocol = decode_type_t::UNKNOWN;
  acState.model    = -1;
  acState.degrees  = 24;
  acState.celsius  = true;
  acState.mode     = stdAc::opmode_t::kCool;
  acState.fanspeed = stdAc::fanspeed_t::kAuto;
  acState.swingv   = stdAc::swingv_t::kOff;
  acState.swingh   = stdAc::swingh_t::kOff;
  acState.turbo    = false;
  acState.econo    = false;
  acState.light    = false;
  acState.filter   = false;
  acState.clean    = false;
  acState.beep     = false;
  acState.sleep    = -1;
  acState.clock    = -1;
}

void IrRemoteHandler::resetConfig() {
  prefs.begin("ac", false);
  prefs.clear();
  prefs.end();

  setDefaultAcState();

  protoName      = "UNKNOWN";
  stateLen       = 0;
  prevStateValid = false;
  scanMode       = false;

  memset(baseState, 0, sizeof(baseState));
}

// ========== SAVE & LOAD ==========
void IrRemoteHandler::saveToFlash() {
  prefs.begin("ac", false);
  prefs.putString("proto",   protoName);
  prefs.putInt("protocol",   (int)acState.protocol);
  prefs.putInt("model",      acState.model);
  prefs.putBool("power",     acState.power);
  prefs.putFloat("temp",     acState.degrees);
  prefs.putBool("celsius",   acState.celsius);
  prefs.putInt("mode",       (int)acState.mode);
  prefs.putInt("fan",        (int)acState.fanspeed);
  prefs.putInt("swingv",     (int)acState.swingv);
  prefs.putInt("swingh",     (int)acState.swingh);
  prefs.putBool("turbo",     acState.turbo);
  prefs.putBool("econo",     acState.econo);
  prefs.putBool("filter",    acState.filter); //todo : tanya ari nanti, beda filter n ion
  prefs.putBool("ion",       acState.filter);
  prefs.putUInt("statelen",  stateLen);
  if (stateLen > 0)
    prefs.putBytes("base", baseState, stateLen);
  prefs.end();
}

void IrRemoteHandler::loadFromFlash() {
  prefs.begin("ac", true);
  protoName            = prefs.getString("proto", "UNKNOWN");
  acState.protocol     = (decode_type_t)prefs.getInt("protocol", (int)decode_type_t::UNKNOWN);
  acState.model        = prefs.getInt("model", -1);
  acState.power        = prefs.getBool("power", false);
  acState.degrees      = prefs.getFloat("temp", 24);
  acState.celsius      = prefs.getBool("celsius", true);
  acState.mode         = (stdAc::opmode_t)prefs.getInt("mode", (int)stdAc::opmode_t::kCool);
  acState.fanspeed     = (stdAc::fanspeed_t)prefs.getInt("fan", (int)stdAc::fanspeed_t::kAuto);
  acState.swingv       = (stdAc::swingv_t)prefs.getInt("swingv", (int)stdAc::swingv_t::kOff);
  acState.swingh       = (stdAc::swingh_t)prefs.getInt("swingh", (int)stdAc::swingh_t::kOff);
  acState.turbo        = prefs.getBool("turbo", false);
  acState.econo        = prefs.getBool("econo", false);
  acState.filter       = prefs.getBool("filter", false);
  acState.light        = false;
  acState.clean        = false;
  acState.beep         = false;
  acState.sleep        = -1;
  acState.clock        = -1;
  stateLen             = prefs.getUInt("statelen", 0);
  if (stateLen > 0)
    prefs.getBytes("base", baseState, stateLen);
  prefs.end();
}

// ========== PRINT STATUS ==========
void IrRemoteHandler::printStatus() {
  Serial.println("========== STATUS AC ==========");
  Serial.print("Protocol : "); Serial.println(protoName);
  Serial.print("Model    : "); Serial.println(acState.model);
  Serial.print("Power    : "); Serial.println(acState.power ? "ON" : "OFF");
  Serial.print("Suhu     : "); Serial.print(acState.degrees); Serial.println(" C");
  Serial.print("Mode     : "); Serial.println(IRac::opmodeToString(acState.mode));
  Serial.print("Fan      : "); Serial.println(IRac::fanspeedToString(acState.fanspeed));
  Serial.print("Swing V  : "); Serial.println(IRac::swingvToString(acState.swingv));
  Serial.print("Swing H  : "); Serial.println(IRac::swinghToString(acState.swingh));
  Serial.print("Turbo    : "); Serial.println(acState.turbo ? "ON" : "OFF");
  Serial.print("Econo    : "); Serial.println(acState.econo ? "ON" : "OFF");
  Serial.print("Data     : "); Serial.println(stateLen > 0 ? "✓ Siap" : "✗ Belum scan");
  Serial.println("================================");
}

// ========== PRINT HELP ==========
void IrRemoteHandler::printHelp() {
  Serial.println("=========== COMMAND ============");
  Serial.println("scan             → scan remote");
  Serial.println("power on/off     → nyala/mati");
  Serial.println("temp 16-30       → set suhu");
  Serial.println("mode cool/heat/dry/fan/auto");
  Serial.println("fan auto/min/low/med/high/max");
  Serial.println("swingv on/off    → swing vertikal");
  Serial.println("swingh on/off    → swing horizontal");
  Serial.println("turbo on/off     → turbo mode");
  Serial.println("econo on/off     → econo mode");
  Serial.println("send             → kirim ulang");
  Serial.println("status           → lihat status");
  Serial.println("reset            → hapus data");
  Serial.println("help             → menu ini");
  Serial.println("================================");
}