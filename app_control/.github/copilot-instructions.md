# Copilot Instructions for KidCar Flutter App

## Project Overview
**KidCar** is a Flutter mobile application targeting Android, iOS, Linux, macOS, and Windows. Currently in early development (v1.0.0) with a minimal starter implementation. The app uses Material Design with Persian UI strings, indicating future internationalization needs.

## Architecture & Project Structure
- **Single-File Architecture**: All UI logic resides in [lib/main.dart](lib/main.dart) - a basic `MyApp` (stateless) with `HomePage` (stateful counter widget)
- **Cross-Platform Support**: Separate native folders (`android/`, `ios/`, `linux/`, `macos/`, `windows/`) with generated plugin registrants
- **Minimal Dependencies**: Only `flutter`, `cupertino_icons`, `flutter_lints` - no state management, HTTP, or persistence packages
- **Web Support**: Includes [web/](web/) folder with `index.html` for web deployment

## Key Development Workflows

### Build Commands
```
flutter run                  # Run on current platform (interactive device selection)
flutter build apk           # Build Android APK
flutter build ios           # Build iOS app
flutter build windows       # Build Windows executable
flutter clean               # Hard reset (required after native code changes)
flutter pub get             # Update dependencies
```

### Code Quality
```
flutter analyze             # Check for lint violations (flutter_lints rules)
flutter format              # Auto-format Dart code (4-space indent)
flutter test                # Run widget tests from test/ directory
```

## Code Patterns & Conventions

### Widget Structure
- `StatelessWidget` for stateless UI, `StatefulWidget` with `State<T>` for mutable state
- Always use `const` constructors (enforced by linter) via `const HomePage()` syntax
- Pattern: [lib/main.dart](lib/main.dart) shows proper widget hierarchy with `MyApp` → `HomePage`

### State Management
- Currently: Plain `setState(() { counter++; })` in `_HomePageState.increase()`
- Migration path: Replace `setState()` with Provider, Riverpod, or Bloc as app complexity grows

### UI & Localization
- Material Design with `ThemeData(primarySwatch: Colors.blue)`
- Persian text hardcoded ("اولین اپ علی 🚀") - implement `Directionality` widget + Flutter i18n when adding localization
- No `MaterialApp` routing configured yet - add `routes` parameter for multi-screen navigation

### Linting
- Follows [analysis_options.yaml](analysis_options.yaml) with `package:flutter_lints/flutter.yaml` rules
- No custom overrides - add to `rules:` section in `analysis_options.yaml` to customize

## Adding New Features

### New Screens
1. Create `StatefulWidget` or `StatelessWidget` in `lib/screens/` (create folder as needed)
2. Import in [lib/main.dart](lib/main.dart) and set as route or home
3. Use pattern from `HomePage` with `Scaffold`, `AppBar`, `FloatingActionButton`

### Dependencies
```
flutter pub add package_name              # Add regular dependency
flutter pub add --dev package_name        # Add dev dependency
```
This regenerates platform-specific registrants automatically.

### Platform-Specific Code
- **Android**: `android/app/src/main/kotlin` (Kotlin) or `java` (legacy)
- **iOS**: `ios/Runner/` (Swift)
- **Windows/macOS/Linux**: CMake configurations in respective folders

## Critical Notes for AI Agents
- **Early Prototype**: Flat, single-file architecture will be refactored soon. Rapid iteration expected.
- **Test Pattern**: [test/widget_test.dart](test/widget_test.dart) shows `WidgetTester` smoke test for counter - follow this pattern for new screens
- **Build Cache Issues**: Always run `flutter clean` after modifying native code (Gradle/Xcode caches prevent updates)
- **Multi-Platform Complexity**: Changes to `android/`, `ios/`, etc. require rebuilds - use `flutter run` to test, not `flutter run --release` (unless testing release mode)
- **No Error Handling**: Hardened exception handling not yet implemented - add defensive patterns when integrating APIs/databases
- **RTL Layout**: Persian text implies right-to-left support needed - test with `Directionality(textDirection: TextDirection.rtl, ...)` when implementing localization
