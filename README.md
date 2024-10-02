# Unofficial REV-Compatible Logger (URCL)

This unofficial project enables automatic logging of CAN traffic from REV motor controllers to NetworkTables or a file, viewable using [AdvantageScope](https://github.com/Mechanical-Advantage/AdvantageScope).

**See the corresponding [AdvantageScope documentation](https://docs.advantagescope.org/more-features/urcl) for more details, including installation and usage instructions.**

## Building and Editing

_Project based on WPILib's [2024 vendor template](https://github.com/wpilibsuite/vendor-template/tree/2024)._

This project uses Gradle with the same base setup as a standard GradleRIO robot project. This means you build with `./gradlew build`, and can install the native toolchain with `./gradlew installRoboRIOToolchain`. If you open this project in VS Code with the wpilib extension installed, you will get intellisense set up for both C++ and Java.

By default, this project builds against the latest WPILib development build. To build against the last WPILib tagged release, build with `./gradlew build -PreleaseMode`.

## Data Format

### Revision 2

- "Raw/Persistent"
  - Buffer Length, Not Published (uint32)
  - Persistent Messages (8 bytes each)
    - Short Message ID (uint16)
    - Data (6 bytes)
- "Raw/Periodic"
  - Buffer Length, Not Published (uint32)
  - Periodic Messages (14 bytes each)
    - Timestamp MS (uint32)
    - Short Message ID (uint16)
    - Data (8 bytes)
- "Raw/Aliases"
  - JSON object of (CAN ID string -> alias string)

### Revision 1

- Buffer Size, Not Published (uint32)
- Persistent Message Count (uint32)
- Periodic Message Count (uint32)
- Persistent Messages (8 bytes each)
  - Short Message ID (uint16)
  - Data (6 bytes)
- Periodic Messages (14 bytes each)
  - Timestamp MS (uint32)
  - Short Message ID (uint16)
  - Data (8 bytes)
