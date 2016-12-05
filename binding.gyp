{
    'targets': [{
        'target_name': 'judy',
        'defines': [ '_LARGEFILE_SOURCE', '_FILE_OFFSET_BITS=64' ],
        'sources': [ 'src/judynode.cc' ],
        'include_dirs': [ '<!(node -e "require(\'nan\')")' ],
        'configurations': {
            'Release': {
                'xcode_settings': {
                    'GCC_OPTIMIZATION_LEVEL': 3
                },
                'cflags': [ '-O3' ],
                'ldflags': [ '-O3' ]
            },
            'Debug': {
                'xcode_settings': {
                    'GCC_OPTIMIZATION_LEVEL': 0
                },
                'cflags': ['-g', '-O0', ],
                'ldflags': ['-g', '-O0']
            }
        }
    }]
}
