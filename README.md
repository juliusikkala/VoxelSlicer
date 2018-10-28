VoxelSlicer
===========

VoxelSlicer slices a 3D model into slices of voxels, saved as PNG images.

## Dependencies

VoxelSlicer depends on EGL, GLEW, GLM, Assimp and the [GNU
C library](https://www.gnu.org/software/libc/). It is therefore advised that
you compile this program using GCC.

In addition to those, VoxelSlicer uses the header-only libraries
[stb\_image](https://github.com/nothings/stb/blob/master/stb_image.h) and
[stb\_image\_write](https://github.com/nothings/stb/blob/master/stb_image_write.h)
for loading textures and writing the image files, respectively. You don't have
to install them manually, since they are entirely contained in `src/`.

## Build

Note that this program has not been tested on macOS and Windows isn't supported
at all.

To build the project using the [Meson build system](https://mesonbuild.com/),
run the following commands:

```sh
meson build
ninja -C build
```

The resulting executable is `build/voxslicer`.

## Usage

```sh
voxslice [-d dimensions] [-o output_prefix] [-i interpolation] [-f fill_type] model_file
```

`dimensions` defines the size of the voxel volume. It can have one of the
following formats:

* WIDTHxHEIGHTxLAYERS
* WIDTHxLAYERS
* LAYERS

WIDTH and HEIGHT define the size of the output image, LAYERS defines the number
of output images.  If either HEIGHT or WIDTH are missing, they are calculated
based on model dimensions such that the aspect ratio is correct.

`output_prefix` is the prefix for all output images. The number of the image
layer and the file extension will be appended.

`interpolation` is the texture interpolation mode used for all textures. It must
be one of the following:

| interpolation | Meaning                 |
|---------------|-------------------------|
| `nearest`     | Nearest neighbor        |
| `linear`      | Bilinear interpolation  |
| `mipmap`      | Trilinear interpolation |

`-f` enables volume filling. This will fill all volume enclosed by the outermost
surfaces. It doesn't detect fully enclosed cavities, so those will be filled as
well. Note that filling may be very slow. It can also fail if the voxelization
contains holes. `fill_type` may be one of the following:

| fill\_type   | Meaning                                       |
|--------------|-----------------------------------------------|
| `flatplus`   | Averages color from neighbors on the XY-plane |
| `volumeplus` | Averages color from neighbors in the volume   |
| `flatx`      | Averages color from neighbors along X-axis    |
| `flaty`      | Averages color from neighbors along Y-axis    |
| `flatz`      | Averages color from neighbors along Z-axis    |

Despite the word 'Averages', this will usually look like nearest-neighbor
interpolation on large-scale images.

`-s` enables single-file output. The output layers are arranged vertically one
after another.

## Supported formats

For the 3D models, all formats supported by Assimp should work. This includes:

* .fbx
* .dae
* .gltf
* .blend
* .obj
* .ply
* .stl

And many more less known formats. For textures, all formats supported by
stb\_image should work. These are:

* PNG
* JPEG
* TGA
* BMP
* PSD (kind of)
* GIF
* HDR
* PIC
* PNM

Exported images are always PNGs.

## Images

Very few slices are used for the example images so that they are not too large.

### Crytek Sponza

```sh
voxslice sponza.obj -d 512x16 -s
```

![sponza](https://user-images.githubusercontent.com/1752365/47613946-79547200-daa0-11e8-90e9-a187acb6fd06.png)

![sponzaslice](https://user-images.githubusercontent.com/1752365/47613753-799f3e00-da9d-11e8-8b84-fd318d7d0bb1.png)

### Infinite head scan

```sh
voxslice head.OBJ -d 512x8 -s -fvolumeplus
```

![lpshead](https://user-images.githubusercontent.com/1752365/47613752-799f3e00-da9d-11e8-89e5-ce7937dd3342.png)

![lpsheadslice](https://user-images.githubusercontent.com/1752365/47613755-799f3e00-da9d-11e8-93af-eec0ecbd25e8.png)

### Stanford Bunny

```sh
voxslice redbunny.obj -d 512x8 -s -fvolumeplus
```

![bunny](https://user-images.githubusercontent.com/1752365/47613754-799f3e00-da9d-11e8-8e75-4a50d8f2e14e.png)

![bunnyslices](https://user-images.githubusercontent.com/1752365/47613756-7a37d480-da9d-11e8-89ed-142b01b30809.png)
