{
  'targets': [
    {
      'target_name': 'judy',
      'cflags': ['-O3', '-D_FILE_OFFSET_BITS=64', '-D_LARGEFILE_SOURCE', '-Wall'],
      'sources': [
      	'judy.cpp'
      ],
      'include_dirs': [
      	'libs/judyarray'
      ]
    }
  ]
}