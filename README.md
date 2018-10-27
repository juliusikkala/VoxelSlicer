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
