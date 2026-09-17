# Third-party code

`catch.hpp` is the original vendored Catch2 v2.13.9 single header. Its original Boost Software License notice is preserved in the file; this maintenance pass does not change or relicense it.

Qt5 is an external, optional build dependency and is not bundled here. No font assets or private course materials are included.

The repository did not contain a project-wide license at the original baseline. This maintenance pass does not select or invent a new license on the author's behalf.

## Optional Windows Qt5 binary package

The GUI links dynamically against unmodified Qt 5.15.2 in the Windows packaging job.
The package carries Qt's LICENSE.LGPLv3, LICENSE.GPL3 and LICENSE.FDL texts, obtained
from the matching upstream tag. Qt libraries and plugins remain separate, replaceable
files. This is not a project-wide license grant for MIPS Simulator.

Upstream corresponding source and notices:
- https://download.qt.io/archive/qt/5.15/5.15.2/single/qt-everywhere-src-5.15.2.tar.xz
- https://github.com/qt/qtbase/tree/v5.15.2
- https://doc.qt.io/qt-5/licensing.html

The compiler runtime deployed by windeployqt retains Microsoft's redistribution
terms. The package uses the SDK deployment tool; it does not relicense its output.
