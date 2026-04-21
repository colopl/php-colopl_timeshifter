# Third-Party Notices

This repository vendors third-party source code under [ext/third_party](ext/third_party).
The project itself is distributed under the BSD-3-Clause license in
[LICENSE](LICENSE). Bundled third-party components remain under their original
licenses and notices.

## timelib

- Location: [ext/third_party/timelib](ext/third_party/timelib)
- Upstream note: timelib is the timezone and date/time library used by PHP's
  Date/Time extension and MongoDB's timezone support.
- License: MIT License. See [ext/third_party/timelib/LICENSE.rst](ext/third_party/timelib/LICENSE.rst).
- Additional notice: [ext/third_party/timelib/parse_posix.c](ext/third_party/timelib/parse_posix.c)
  states that part of the implementation is adapted from IANA tzcode and is
  public-domain licensed.

The MIT license used by timelib is permissive and compatible with distributing
this repository under BSD-3-Clause, provided the original timelib notices are
retained in source and accompanying materials.
