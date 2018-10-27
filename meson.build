project(
  'VoxelSlicer',
  ['cpp'],
  default_options : ['cpp_std=c++1z', 'buildtype=debugoptimized']
)

src = [
  'src/main.cc',
  'src/model.cc',
  'src/shader.cc',
  'src/stb_image.cc',
  'src/stb_image_write.cc',
]

cc = meson.get_compiler('cpp')
m_dep = cc.find_library('m', required : false)
egl_dep = dependency('egl')
glew_dep = dependency('glew')
glm_dep = dependency('glm')
assimp_dep = dependency('assimp')

incdir = include_directories('src')

executable(
  'voxslice',
  src,
  dependencies: [
    m_dep,
    egl_dep,
    glm_dep,
    glew_dep,
    assimp_dep,
  ],
  include_directories: [incdir],
  install: true,
)

