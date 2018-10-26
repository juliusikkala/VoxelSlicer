project(
  'VoxelSlicer',
  ['cpp'],
  default_options : ['cpp_std=c++1z', 'buildtype=debugoptimized']
)

src = [
  'src/main.cpp',
  'src/stb_image.cpp',
  'src/stb_image_write.cpp',
]

cc = meson.get_compiler('cpp')
m_dep = cc.find_library('m', required : false)
egl_dep = dependency('egl')
glew_dep = dependency('glew')
glm_dep = dependency('glm')

incdir = include_directories('src')

executable(
  'voxslice',
  src,
  dependencies: [
    m_dep,
    egl_dep,
    glm_dep,
    glew_dep,
  ],
  include_directories: [incdir],
  install: true,
)

