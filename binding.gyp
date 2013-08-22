{
  'targets': [
    {
      'target_name': 'judy',
      'cflags': [ '-O3', '-D_FILE_OFFSET_BITS=64', '-D_LARGEFILE_SOURCE', '-Wall' ],
      'sources': [ 'src/judy.cc' ]
    }
  ]
}