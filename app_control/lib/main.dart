import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:wakelock_plus/wakelock_plus.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  WakelockPlus.enable();
  await SystemChrome.setPreferredOrientations([
    DeviceOrientation.landscapeLeft,
    DeviceOrientation.landscapeRight,
  ]);
  await SystemChrome.setEnabledSystemUIMode(SystemUiMode.immersiveSticky);
  SystemChrome.setSystemUIOverlayStyle(
    const SystemUiOverlayStyle(
      statusBarColor: Colors.transparent,
      systemNavigationBarColor: Colors.transparent,
      statusBarIconBrightness: Brightness.light,
      systemNavigationBarIconBrightness: Brightness.light,
    ),
  );
  runApp(const KidCarApp());
}

enum AppLang { fa, en }

class KidCarApp extends StatefulWidget {
  const KidCarApp({super.key});

  @override
  State<KidCarApp> createState() => _KidCarAppState();
}

class _KidCarAppState extends State<KidCarApp> {
  AppLang? _lang;

  void _selectLang(AppLang lang) {
    setState(() {
      _lang = lang;
    });
  }

  void _resetLang() {
    setState(() {
      _lang = null;
    });
  }

  @override
  Widget build(BuildContext context) {
    final lang = _lang;
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'KidCar Control',
      theme: ThemeData(
        useMaterial3: false,
        scaffoldBackgroundColor: const Color(0xFFF5F7FA),
        colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xFF0B6E8E)),
        sliderTheme: const SliderThemeData(
          activeTrackColor: Color(0xFF0B6E8E),
          inactiveTrackColor: Color(0xFFCBD5E1),
          thumbColor: Color(0xFF0B6E8E),
          overlayColor: Color(0x330B6E8E),
        ),
      ),
      builder: (context, child) {
        // Keep physical layout identical in both languages; only labels change.
        const direction = TextDirection.ltr;
        return Directionality(
          textDirection: direction,
          child: child ?? const SizedBox(),
        );
      },
      home: (lang == null)
          ? LanguageSelectScreen(onSelect: _selectLang)
          : ControlScreen(lang: lang, onChangeLanguage: _resetLang),
    );
  }
}

class LanguageSelectScreen extends StatelessWidget {
  const LanguageSelectScreen({super.key, required this.onSelect});

  final void Function(AppLang lang) onSelect;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            colors: [Color(0xFF0B6E8E), Color(0xFF5AB1BB)],
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
          ),
        ),
        child: Center(
          child: ConstrainedBox(
            constraints: const BoxConstraints(maxWidth: 720),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                const Text(
                  'Choose Language / انتخاب زبان',
                  style: TextStyle(
                    fontSize: 28,
                    fontWeight: FontWeight.bold,
                    color: Colors.white,
                  ),
                ),
                const SizedBox(height: 32),
                LayoutBuilder(
                  builder: (context, constraints) {
                    final maxW = constraints.maxWidth;
                    final buttonW = maxW < 420 ? (maxW / 2) - 16 : 200.0;
                    return Wrap(
                      alignment: WrapAlignment.center,
                      spacing: 16,
                      runSpacing: 16,
                      children: [
                        _LangButton(
                          label: 'فارسي',
                          width: buttonW,
                          onTap: () => onSelect(AppLang.fa),
                        ),
                        _LangButton(
                          label: 'English',
                          width: buttonW,
                          onTap: () => onSelect(AppLang.en),
                        ),
                      ],
                    );
                  },
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _LangButton extends StatelessWidget {
  const _LangButton({
    required this.label,
    required this.onTap,
    required this.width,
  });

  final String label;
  final VoidCallback onTap;
  final double width;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: width,
      height: 70,
      child: ElevatedButton(
        onPressed: onTap,
        style: ElevatedButton.styleFrom(
          backgroundColor: Colors.white,
          foregroundColor: const Color(0xFF0B6E8E),
          textStyle: const TextStyle(fontSize: 22, fontWeight: FontWeight.bold),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(14),
          ),
        ),
        child: Text(label),
      ),
    );
  }
}

class UdpSender {
  UdpSender({required this.host, required this.port, required this.onMessage});

  final String host;
  final int port;
  final void Function(String data, InternetAddress address)? onMessage;

  RawDatagramSocket? _socket;
  InternetAddress? _address;
  set address(InternetAddress addr) => _address = addr;

  Future<void> init() async {
    _socket ??= await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
    _socket!.broadcastEnabled = true;
    _address ??= InternetAddress(host);
    _socket!.listen((event) {
      if (event == RawSocketEvent.read) {
        final datagram = _socket!.receive();
        if (datagram != null) {
          final msg = utf8.decode(datagram.data);
          onMessage?.call(msg, datagram.address);
        }
      }
    });
  }

  void send(Map<String, dynamic> payload) {
    if (_socket == null || _address == null) return;
    final data = utf8.encode(jsonEncode(payload));
    try {
      _socket!.send(data, _address!, port);
    } catch (_) {}
  }

  void sendTo(InternetAddress addr, Map<String, dynamic> payload) {
    if (_socket == null) return;
    final data = utf8.encode(jsonEncode(payload));
    try {
      _socket!.send(data, addr, port);
    } catch (_) {}
  }

  void dispose() {
    _socket?.close();
    _socket = null;
  }

  Future<void> reset() async {
    dispose();
    await init();
  }
}

class ControlScreen extends StatefulWidget {
  const ControlScreen({
    super.key,
    required this.lang,
    required this.onChangeLanguage,
  });

  final AppLang lang;
  final VoidCallback onChangeLanguage;

  @override
  State<ControlScreen> createState() => _ControlScreenState();
}

class _ControlScreenState extends State<ControlScreen>
    with WidgetsBindingObserver {
  static const MethodChannel _wifiChannel = MethodChannel('kidcar/wifi');
  static const MethodChannel _powerChannel = MethodChannel('kidcar/wifi');
  static const int _minSpeed = 20;
  int _speed = 50; // 20..100
  int _throttle = 0; // -100..100
  int _steer = 0; // -100..100
  bool _parked = true;
  bool _forwardPressed = false;
  bool _backPressed = false;
  bool _leftPressed = false;
  bool _rightPressed = false;
  bool _parkBlinkOn = false;
  static const Duration _steerHoldLimit = Duration(seconds: 5);
  DateTime? _steerHoldStartedAt;
  bool _steerHoldLockedUntilRelease = false;
  int _steerHoldDirection = 0; // -1 left, 1 right
  Timer? _parkBlinkTimer;
  Timer? _txTimer;
  Timer? _connectProbeTimer;

  final int _battery = 78; // demo
  int _signal = 66; // updated from Wi-Fi RSSI
  bool _connected = false;
  DateTime _lastAck = DateTime.fromMillisecondsSinceEpoch(0);
  String _espStatus = 'CHECKING';
  InternetAddress? _espAddress;
  int _txCount = 0;
  DateTime _lastTx = DateTime.fromMillisecondsSinceEpoch(0);

  DateTime _now = DateTime.now();
  Timer? _clockTimer;
  Timer? _heartbeatTimer;

  late final UdpSender _udp = UdpSender(
    host: '255.255.255.255',
    port: 4210,
    onMessage: _handleUdpMessage,
  );

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    SystemChrome.setEnabledSystemUIMode(SystemUiMode.immersiveSticky);
    WakelockPlus.enable();
    _requestIgnoreBatteryOptimizations();
    _udp.init();
    _startConnectProbe();
    _sendState();
    _clockTimer = Timer.periodic(const Duration(seconds: 30), (_) {
      setState(() => _now = DateTime.now());
    });
    _heartbeatTimer = Timer.periodic(const Duration(milliseconds: 500), (_) {
      _sendState();
    });
  }

  Future<void> _requestIgnoreBatteryOptimizations() async {
    try {
      await _powerChannel.invokeMethod<bool>('requestIgnoreBatteryOptimizations');
    } catch (_) {}
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    _clockTimer?.cancel();
    _parkBlinkTimer?.cancel();
    _txTimer?.cancel();
    _connectProbeTimer?.cancel();
    _heartbeatTimer?.cancel();
    WakelockPlus.disable();
    _udp.dispose();
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) {
      _reconnectAfterResume();
    }
  }

  Future<void> _reconnectAfterResume() async {
    _espAddress = null;
    _lastAck = DateTime.fromMillisecondsSinceEpoch(0);
    if (mounted) {
      setState(() {
        _connected = false;
        _signal = 0;
        _espStatus = 'CHECKING';
      });
    }
    await _udp.reset();
    _startConnectProbe();
    _sendState();
  }

  void _startConnectProbe() {
    _connectProbeTimer?.cancel();
    _sendTestUdp();
    _connectProbeTimer = Timer.periodic(const Duration(milliseconds: 500), (_) {
      if (_connected) {
        _connectProbeTimer?.cancel();
        _connectProbeTimer = null;
        return;
      }
      _sendTestUdp();
    });
  }

  bool _anyPressed() {
    return _forwardPressed || _backPressed || _leftPressed || _rightPressed;
  }

  void _startTxLoop() {
    if (_txTimer != null) return;
    _txTimer = Timer.periodic(const Duration(milliseconds: 100), (_) {
      if (_parked || !_anyPressed()) {
        _txTimer?.cancel();
        _txTimer = null;
        return;
      }
      _sendState();
    });
  }

  void _stopTxLoop() {
    _txTimer?.cancel();
    _txTimer = null;
  }

  void _blinkPark() {
    _parkBlinkTimer?.cancel();
    int ticks = 0;
    _parkBlinkTimer = Timer.periodic(const Duration(milliseconds: 90), (t) {
      ticks++;
      setState(() => _parkBlinkOn = !_parkBlinkOn);
      if (ticks >= 6) {
        t.cancel();
        setState(() => _parkBlinkOn = false);
      }
    });
  }

  void _beep() {
    SystemSound.play(SystemSoundType.alert);
  }

  void _handleUdpMessage(String data, InternetAddress address) {
    try {
      final obj = jsonDecode(data);
      if (obj is Map && obj['ok'] == 1) {
        _lastAck = DateTime.now();
        final status = (obj['status'] ?? 'OK').toString().toUpperCase();
        if (!_connected || _signal < 80 || _espStatus != status) {
          setState(() {
            _connected = true;
            _signal = 80;
            _espStatus = status;
            _espAddress = address;
          });
          _udp.address = address;
        } else if (_espAddress == null ||
            _espAddress!.address != address.address) {
          _espAddress = address;
          _udp.address = address;
        }
      }
    } catch (_) {
      // ignore parse errors
    }
  }

  Map<String, dynamic> _controlPayload({bool includePing = false}) {
    final payload = {
      'throttle': _throttle,
      'steer': _steer,
      'speed': _speed,
      'park': _parked,
      'battery': _battery,
      'signal': _signal,
    };
    if (includePing) {
      payload['ping'] = 1;
      payload['test'] = 'kidcar';
      payload['ts'] = DateTime.now().millisecondsSinceEpoch;
    }
    return payload;
  }

  void _sendState() {
    final msSinceAck = DateTime.now().difference(_lastAck).inMilliseconds;
    final isConnected = msSinceAck < 3000;
    if (isConnected != _connected) {
      setState(() {
        _connected = isConnected;
        if (!isConnected) _signal = 0;
        if (!isConnected) _espStatus = 'CHECKING';
      });
      if (!isConnected) {
        _startConnectProbe();
      }
    }
    if (!isConnected) {
      _espAddress = null;
      _udp.address = InternetAddress('255.255.255.255');
    } else if (_espAddress != null) {
      _udp.address = _espAddress!;
    }

    _txCount += 1;
    _lastTx = DateTime.now();
    debugPrint('TX $_txCount @ ${_lastTx.toIso8601String()}');
    final payload = _controlPayload();

    if (_espAddress != null) {
      _udp.sendTo(_espAddress!, payload);
    } else {
      _udp.sendTo(InternetAddress('255.255.255.255'), payload);
      _udp.sendTo(InternetAddress('192.168.4.255'), payload);
      _udp.sendTo(InternetAddress('192.168.4.1'), payload);
    }
  }

  Future<void> _showWifiName() async {
    String name = '';
    try {
      final result = await _wifiChannel.invokeMethod<String>('getWifiName');
      name = (result ?? '').replaceAll('"', '');
    } catch (_) {
      name = '';
    }
    if (name.isEmpty) {
      name = (widget.lang == AppLang.fa) ? 'نامشخص' : 'Unknown';
    }
    if (!mounted) return;
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text((widget.lang == AppLang.fa) ? 'شبکه وای‌فای' : 'Wi‑Fi Network'),
        content: Text(name),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: Text((widget.lang == AppLang.fa) ? 'باشه' : 'OK'),
          ),
        ],
      ),
    );
  }

  void _sendTestUdp() {
    final payload = _controlPayload(includePing: true);
    _udp.sendTo(InternetAddress('192.168.4.1'), payload);
    _udp.sendTo(InternetAddress('192.168.4.255'), payload);
    _udp.sendTo(InternetAddress('255.255.255.255'), payload);
  }

  String t(String key) {
    const fa = {
      'forward': 'جلو',
      'back': 'عقب',
      'left': 'چپ',
      'right': 'راست',
      'speed': 'سرعت',
      'park': 'پارک',
      'hold_to_drive': 'براي حرکت، دکمه پارک را نگه داريد',
      'battery': 'باتري',
      'signal': 'آنتن',
      'drive_locked': 'حرکت قفل است',
      'drive_unlocked': 'حرکت فعال شد',
      'app_title': 'کنترل ماشين کودک',
      'connected': 'متصل',
      'disconnected': 'قطع',
      'checking': 'در حال بررسی',
      'ok': 'سالم',
      'fault': 'خطا',
    };

    const en = {
      'forward': 'Forward',
      'back': 'Back',
      'left': 'Left',
      'right': 'Right',
      'speed': 'Speed',
      'park': 'PARK',
      'hold_to_drive': 'Hold PARK to drive',
      'battery': 'Battery',
      'signal': 'Signal',
      'drive_locked': 'Drive locked',
      'drive_unlocked': 'Drive unlocked',
      'app_title': 'KidCar Control',
      'connected': 'Connected',
      'disconnected': 'Disconnected',
      'checking': 'Checking',
      'ok': 'OK',
      'fault': 'Fault',
    };

    final map = (widget.lang == AppLang.fa) ? fa : en;
    return map[key] ?? key;
  }

  void _applyMotion() {
    int throttle = 0;
    if (_forwardPressed && !_backPressed) throttle = _speed;
    if (_backPressed && !_forwardPressed) throttle = -_speed;
    int steer = 0;
    if (_leftPressed && !_rightPressed) steer = -_speed;
    if (_rightPressed && !_leftPressed) steer = _speed;

    final int steerDir = steer > 0 ? 1 : (steer < 0 ? -1 : 0);
    if (steerDir == 0) {
      _steerHoldStartedAt = null;
      _steerHoldLockedUntilRelease = false;
      _steerHoldDirection = 0;
    } else if (_steerHoldLockedUntilRelease) {
      steer = 0;
    } else {
      final now = DateTime.now();
      if (_steerHoldDirection != steerDir || _steerHoldStartedAt == null) {
        _steerHoldDirection = steerDir;
        _steerHoldStartedAt = now;
      } else if (now.difference(_steerHoldStartedAt!) >= _steerHoldLimit) {
        _steerHoldLockedUntilRelease = true;
        steer = 0;
      }
    }

    setState(() {
      _throttle = throttle;
      _steer = steer;
    });
    _sendState();
    if (_parked || !_anyPressed()) {
      _stopTxLoop();
    } else {
      _startTxLoop();
    }
  }

  void _forwardDown() {
    if (_parked) {
      _blinkPark();
      _beep();
      return;
    }
    _forwardPressed = true;
    _applyMotion();
  }

  void _forwardUp() {
    _forwardPressed = false;
    _applyMotion();
  }

  void _backDown() {
    if (_parked) {
      _blinkPark();
      _beep();
      return;
    }
    _backPressed = true;
    _applyMotion();
  }

  void _backUp() {
    _backPressed = false;
    _applyMotion();
  }

  void _leftDown() {
    if (_parked) {
      _blinkPark();
      _beep();
      return;
    }
    _leftPressed = true;
    _applyMotion();
  }

  void _leftUp() {
    _leftPressed = false;
    _applyMotion();
  }

  void _rightDown() {
    if (_parked) {
      _blinkPark();
      _beep();
      return;
    }
    _rightPressed = true;
    _applyMotion();
  }

  void _rightUp() {
    _rightPressed = false;
    _applyMotion();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            colors: [Color(0xFFF5F7FA), Color(0xFFE8EEF3)],
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
          ),
        ),
        child: SafeArea(
          child: Column(
            children: [
              StatusBarWidget(
                title: t('app_title'),
                battery: _battery,
                signal: _signal,
                connected: _connected,
                connectedLabel: _connected ? t('connected') : t('disconnected'),
                espStatusLabel: _espStatus == 'OK'
                    ? t('ok')
                    : _espStatus == 'FAULT'
                    ? t('fault')
                    : t('checking'),
                lang: widget.lang,
                now: _now,
                onChangeLanguage: widget.onChangeLanguage,
                onWifiTap: _showWifiName,
              ),
              const SizedBox(height: 8),
              Expanded(
                child: Padding(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 16,
                    vertical: 8,
                  ),
                  child: Row(
                    children: [
                      Expanded(
                        child: ControlPad(
                          primaryLabel: t('left'),
                          secondaryLabel: t('right'),
                          primaryIcon: Icons.arrow_back,
                          secondaryIcon: Icons.arrow_forward,
                          onPrimaryDown: _leftDown,
                          onPrimaryUp: _leftUp,
                          onSecondaryDown: _rightDown,
                          onSecondaryUp: _rightUp,
                          isLeft: true,
                        ),
                      ),
                      Expanded(
                        child: CenterPanel(
                          speed: _speed,
                          minSpeed: _minSpeed,
                          onSpeedChanged: (v) {
                            setState(() => _speed = v);
                            _sendState();
                          },
                          parked: _parked,
                          parkBlink: _parkBlinkOn,
                          onParkToggle: () {
                            setState(() {
                              _parked = !_parked;
                              if (_parked) {
                                _forwardPressed = false;
                                _backPressed = false;
                                _leftPressed = false;
                                _rightPressed = false;
                              }
                            });
                            if (_parked) {
                              _stopTxLoop();
                              _applyMotion();
                            } else {
                              _sendState();
                            }
                          },
                          parkLabel: t('park'),
                          parkHint: t('hold_to_drive'),
                          statusText: _parked
                              ? t('drive_locked')
                              : t('drive_unlocked'),
                        ),
                      ),
                      Expanded(
                        child: ControlPad(
                          primaryLabel: t('forward'),
                          secondaryLabel: t('back'),
                          primaryIcon: Icons.arrow_upward,
                          secondaryIcon: Icons.arrow_downward,
                          onPrimaryDown: _forwardDown,
                          onPrimaryUp: _forwardUp,
                          onSecondaryDown: _backDown,
                          onSecondaryUp: _backUp,
                          isLeft: false,
                        ),
                      ),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class StatusBarWidget extends StatelessWidget {
  const StatusBarWidget({
    super.key,
    required this.title,
    required this.battery,
    required this.signal,
    required this.connected,
    required this.connectedLabel,
    required this.espStatusLabel,
    required this.lang,
    required this.now,
    required this.onChangeLanguage,
    required this.onWifiTap,
  });

  final String title;
  final int battery;
  final int signal;
  final bool connected;
  final String connectedLabel;
  final String espStatusLabel;
  final AppLang lang;
  final DateTime now;
  final VoidCallback onChangeLanguage;
  final VoidCallback onWifiTap;

  String _timeText() {
    final h = now.hour.toString().padLeft(2, '0');
    final m = now.minute.toString().padLeft(2, '0');
    return '$h:$m';
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      height: 52,
      padding: const EdgeInsets.symmetric(horizontal: 16),
      decoration: const BoxDecoration(
        color: Color(0xFF0B6E8E),
        boxShadow: [
          BoxShadow(color: Colors.black26, blurRadius: 6, offset: Offset(0, 2)),
        ],
      ),
      child: Row(
        children: [
          const SizedBox(width: 12),
          Text(
            connectedLabel,
            style: TextStyle(
              color: connected
                  ? const Color(0xFF2ECC71)
                  : const Color(0xFFE74C3C),
              fontSize: 12,
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(width: 18),
          Text(
            (lang == AppLang.fa) ? 'وضعیت:' : 'Status:',
            style: const TextStyle(
              color: Colors.white70,
              fontSize: 12,
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(width: 6),
          Text(
            espStatusLabel,
            style: const TextStyle(
              color: Colors.white70,
              fontSize: 12,
              fontWeight: FontWeight.w600,
            ),
          ),
          Expanded(
            child: Center(
              child: Text(
                title,
                style: const TextStyle(
                  color: Colors.white,
                  fontSize: 18,
                  fontWeight: FontWeight.w600,
                ),
              ),
            ),
          ),
          Text(
            _timeText(),
            style: const TextStyle(
              color: Colors.white70,
              fontSize: 14,
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(width: 16),
          GestureDetector(
            onTap: onWifiTap,
            child: Icon(_wifiIcon(signal), color: _levelColor(signal), size: 20),
          ),
          const SizedBox(width: 12),
          BatteryIcon(level: battery),
          const SizedBox(width: 6),
          Text(
            '$battery%',
            style: const TextStyle(color: Colors.white70, fontSize: 12),
          ),
          const SizedBox(width: 16),
          IconButton(
            onPressed: onChangeLanguage,
            icon: const Icon(Icons.translate, color: Colors.white),
            tooltip: (lang == AppLang.fa) ? 'زبان' : 'Language',
          ),
        ],
      ),
    );
  }

  Color _levelColor(int level) {
    if (level <= 20) return const Color(0xFFE74C3C);
    if (level <= 50) return const Color(0xFFF1C40F);
    return const Color(0xFF2ECC71);
  }

  IconData _wifiIcon(int level) {
    if (level <= 25) return Icons.wifi_1_bar;
    if (level <= 50) return Icons.wifi_2_bar;
    return Icons.wifi;
  }
}

class BatteryIcon extends StatelessWidget {
  const BatteryIcon({super.key, required this.level});

  final int level;

  Color _color(int level) {
    if (level <= 20) return const Color(0xFFE74C3C);
    if (level <= 50) return const Color(0xFFF1C40F);
    return const Color(0xFF2ECC71);
  }

  @override
  Widget build(BuildContext context) {
    final pct = level.clamp(0, 100);
    return Container(
      width: 32,
      height: 14,
      padding: const EdgeInsets.all(2),
      decoration: BoxDecoration(
        border: Border.all(color: Colors.white70),
        borderRadius: BorderRadius.circular(3),
      ),
      child: Stack(
        children: [
          Align(
            alignment: Alignment.centerLeft,
            child: Container(
              width: (pct / 100) * 26,
              decoration: BoxDecoration(
                color: _color(pct),
                borderRadius: BorderRadius.circular(2),
              ),
            ),
          ),
          Positioned(
            right: -4,
            top: 3,
            child: Container(
              width: 3,
              height: 6,
              decoration: BoxDecoration(
                color: Colors.white70,
                borderRadius: BorderRadius.circular(1),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

// SignalBars removed in favor of a mobile-style Wi-Fi icon.

class ControlPad extends StatelessWidget {
  const ControlPad({
    super.key,
    required this.primaryLabel,
    required this.secondaryLabel,
    required this.primaryIcon,
    required this.secondaryIcon,
    required this.onPrimaryDown,
    required this.onPrimaryUp,
    required this.onSecondaryDown,
    required this.onSecondaryUp,
    required this.isLeft,
  });

  final String primaryLabel;
  final String secondaryLabel;
  final IconData primaryIcon;
  final IconData secondaryIcon;
  final VoidCallback onPrimaryDown;
  final VoidCallback onPrimaryUp;
  final VoidCallback onSecondaryDown;
  final VoidCallback onSecondaryUp;
  final bool isLeft;

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (context, constraints) {
        if (isLeft) {
          final gap = 18.0;
          final rawWidth = (constraints.maxWidth - gap) / 2;
          final buttonWidth = rawWidth < 90.0
              ? rawWidth
              : (rawWidth > 150.0 ? 150.0 : rawWidth);
          final rawHeight = constraints.maxHeight * 0.4;
          final buttonHeight = rawHeight < 56.0
              ? rawHeight
              : (rawHeight > 120.0 ? 120.0 : rawHeight);

          return Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              ControlButton(
                icon: primaryIcon,
                label: primaryLabel,
                onPress: onPrimaryDown,
                onRelease: onPrimaryUp,
                color: const Color(0xFF0B6E8E),
                width: buttonWidth,
                height: buttonHeight,
              ),
              SizedBox(width: gap),
              ControlButton(
                icon: secondaryIcon,
                label: secondaryLabel,
                onPress: onSecondaryDown,
                onRelease: onSecondaryUp,
                color: const Color(0xFF159AAE),
                width: buttonWidth,
                height: buttonHeight,
              ),
            ],
          );
        }

        final gap = 18.0;
        final rawHeight = (constraints.maxHeight - gap) / 2;
        final buttonHeight = rawHeight < 52.0
            ? rawHeight
            : (rawHeight > 120.0 ? 120.0 : rawHeight);
        final rawWidth = constraints.maxWidth * 0.8;
        final buttonWidth = rawWidth < 120.0
            ? rawWidth
            : (rawWidth > 200.0 ? 200.0 : rawWidth);

        return Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            ControlButton(
              icon: primaryIcon,
              label: primaryLabel,
              onPress: onPrimaryDown,
              onRelease: onPrimaryUp,
              color: const Color(0xFF0B6E8E),
              width: buttonWidth,
              height: buttonHeight,
            ),
            SizedBox(height: gap),
            ControlButton(
              icon: secondaryIcon,
              label: secondaryLabel,
              onPress: onSecondaryDown,
              onRelease: onSecondaryUp,
              color: const Color(0xFF159AAE),
              width: buttonWidth,
              height: buttonHeight,
            ),
          ],
        );
      },
    );
  }
}

class ControlButton extends StatefulWidget {
  const ControlButton({
    super.key,
    required this.icon,
    required this.label,
    required this.onPress,
    required this.onRelease,
    required this.color,
    required this.width,
    required this.height,
  });

  final IconData icon;
  final String label;
  final VoidCallback onPress;
  final VoidCallback onRelease;
  final Color color;
  final double width;
  final double height;

  @override
  State<ControlButton> createState() => _ControlButtonState();
}

class _ControlButtonState extends State<ControlButton> {
  bool _pressed = false;
  int _activePointers = 0;

  void _setPressed(bool value) {
    if (_pressed == value) return;
    setState(() => _pressed = value);
  }

  @override
  Widget build(BuildContext context) {
    final pressScale = _pressed ? 0.97 : 1.0;
    final overlayOpacity = _pressed ? 0.2 : 0.0;

    return Listener(
      onPointerDown: (_) {
        _activePointers += 1;
        _setPressed(true);
        widget.onPress();
      },
      onPointerUp: (_) {
        _activePointers = (_activePointers - 1).clamp(0, 999999);
        if (_activePointers == 0) {
          _setPressed(false);
          widget.onRelease();
        }
      },
      onPointerCancel: (_) {
        _activePointers = (_activePointers - 1).clamp(0, 999999);
        if (_activePointers == 0) {
          _setPressed(false);
          widget.onRelease();
        }
      },
      child: AnimatedScale(
        scale: pressScale,
        duration: const Duration(milliseconds: 80),
        curve: Curves.easeOut,
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 120),
          width: widget.width,
          height: widget.height,
          decoration: BoxDecoration(
            color: widget.color,
            borderRadius: BorderRadius.circular(18),
            boxShadow: const [
              BoxShadow(
                color: Colors.black26,
                blurRadius: 10,
                offset: Offset(0, 4),
              ),
            ],
          ),
          child: Stack(
            children: [
              Positioned.fill(
                child: Align(
                  alignment: Alignment.center,
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Icon(
                        widget.icon,
                        size: widget.height < 70 ? 26 : 34,
                        color: Colors.white,
                      ),
                      const SizedBox(height: 6),
                      Text(
                        widget.label,
                        style: const TextStyle(
                          color: Colors.white,
                          fontSize: 18,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                    ],
                  ),
                ),
              ),
              Positioned.fill(
                child: IgnorePointer(
                  child: AnimatedOpacity(
                    opacity: overlayOpacity,
                    duration: const Duration(milliseconds: 120),
                    child: Container(
                      decoration: BoxDecoration(
                        color: Colors.white,
                        borderRadius: BorderRadius.circular(18),
                      ),
                    ),
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class CenterPanel extends StatelessWidget {
  const CenterPanel({
    super.key,
    required this.speed,
    required this.minSpeed,
    required this.onSpeedChanged,
    required this.parked,
    required this.parkBlink,
    required this.onParkToggle,
    required this.parkLabel,
    required this.parkHint,
    required this.statusText,
  });

  final int speed;
  final int minSpeed;
  final ValueChanged<int> onSpeedChanged;
  final bool parked;
  final bool parkBlink;
  final VoidCallback onParkToggle;
  final String parkLabel;
  final String parkHint;
  final String statusText;

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (context, constraints) {
        final panelWidth = constraints.maxWidth;
        final panelHeight = constraints.maxHeight;

        final gaugeWidth = panelWidth < 220.0
            ? panelWidth
            : (panelWidth > 320.0 ? 320.0 : panelWidth);
        final rawGaugeHeight = panelHeight * 0.22;
        final gaugeHeight = rawGaugeHeight < 70.0
            ? rawGaugeHeight
            : (rawGaugeHeight > 110.0 ? 110.0 : rawGaugeHeight);

        final sliderBase = panelWidth * 0.9;
        final sliderWidth = sliderBase < 200.0
            ? sliderBase
            : (sliderBase > 320.0 ? 320.0 : sliderBase);

        final parkBase = panelWidth * 0.7;
        final parkWidth = panelWidth < 150.0
            ? parkBase
            : (panelWidth > 200.0 ? 180.0 : parkBase);
        final rawParkHeight = panelHeight * 0.18;
        final parkHeight = rawParkHeight < 56.0
            ? rawParkHeight
            : (rawParkHeight > 78.0 ? 78.0 : rawParkHeight);

        return Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            SpeedGauge(speed: speed, width: gaugeWidth, height: gaugeHeight),
            const SizedBox(height: 10),
            SizedBox(
              width: sliderWidth,
              child: Slider(
                value: speed.toDouble(),
                min: minSpeed.toDouble(),
                max: 100,
                divisions: 100 - minSpeed,
                label: '$speed',
                onChanged: (v) {
                  final next = v.round() < minSpeed ? minSpeed : v.round();
                  onSpeedChanged(next);
                },
              ),
            ),
            Text(
              statusText,
              style: TextStyle(
                color: parked
                    ? const Color(0xFFE74C3C)
                    : const Color(0xFF2ECC71),
                fontWeight: FontWeight.w600,
              ),
            ),
            const SizedBox(height: 8),
            GestureDetector(
              onLongPress: onParkToggle,
              child: Container(
                width: parkWidth,
                height: parkHeight,
                decoration: BoxDecoration(
                  color: parked
                      ? (parkBlink
                            ? const Color(0xFFFF9AA2)
                            : const Color(0xFFE74C3C))
                      : const Color(0xFF2ECC71),
                  borderRadius: BorderRadius.circular(18),
                  boxShadow: const [
                    BoxShadow(
                      color: Colors.black26,
                      blurRadius: 10,
                      offset: Offset(0, 4),
                    ),
                  ],
                ),
                child: Center(
                  child: Text(
                    parkLabel,
                    style: const TextStyle(
                      color: Colors.white,
                      fontSize: 20,
                      fontWeight: FontWeight.bold,
                      letterSpacing: 1.2,
                    ),
                  ),
                ),
              ),
            ),
            const SizedBox(height: 6),
            Text(
              parkHint,
              style: const TextStyle(fontSize: 12, color: Colors.black54),
            ),
          ],
        );
      },
    );
  }
}

class SpeedGauge extends StatelessWidget {
  const SpeedGauge({
    super.key,
    required this.speed,
    required this.width,
    required this.height,
  });

  final int speed;
  final double width;
  final double height;

  @override
  Widget build(BuildContext context) {
    final pct = speed.clamp(0, 100);
    return Container(
      width: width,
      height: height,
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(16),
        boxShadow: const [
          BoxShadow(color: Colors.black12, blurRadius: 8, offset: Offset(0, 4)),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              const Text('0'),
              Text(
                pct.toString(),
                style: const TextStyle(
                  fontWeight: FontWeight.bold,
                  fontSize: 18,
                ),
              ),
              const Text('100'),
            ],
          ),
          const SizedBox(height: 8),
          Expanded(
            child: Stack(
              children: [
                Container(
                  decoration: BoxDecoration(
                    borderRadius: BorderRadius.circular(10),
                    gradient: const LinearGradient(
                      colors: [
                        Color(0xFF2ECC71),
                        Color(0xFFF1C40F),
                        Color(0xFFE74C3C),
                      ],
                    ),
                  ),
                ),
                FractionallySizedBox(
                  widthFactor: pct / 100,
                  child: Container(
                    decoration: BoxDecoration(
                      color: const Color.fromARGB(51, 255, 255, 255),
                      borderRadius: BorderRadius.circular(10),
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
