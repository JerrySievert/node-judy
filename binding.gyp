{
  "targets": [{
    "target_name": "judy",
    "type": "static_library",
    "defines": [ "_LARGEFILE_SOURCE", "_FILE_OFFSET_BITS=64" ],
    "include_dirs": "src",
    "sources": [ "src/judy64nb.c", "src/judynode.cc" ],
    "include_dirs": [ "<!(node -e \"require('nan')\")" ],
    "cflags": [ "-O3", "-fPIC", "-DNDEBUG", "-Wall", "-Wextra", "-Wshadow", "-Wnon-virtual-dtor", "-pedantic" ],
    "cflags_cc": [ "-O3", "-fPIC", "-DNDEBUG", "-Wall", "-Wextra", "-Wshadow", "-Wnon-virtual-dtor", "-pedantic" ]
  }]
}
