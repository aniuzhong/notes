# 色彩管理

- [色彩管理](#色彩管理)
  - [Adobe APP14 segment](#adobe-app14-segment)
  - [libjpeg 解码器对 APP14 的处理](#libjpeg-解码器对-app14-的处理)
    - [1. 输入与输出颜色空间的推断](#1-输入与输出颜色空间的推断)
      - [情况一：单通道（灰度图）](#情况一单通道灰度图)
      - [情况二：三通道（常见的彩色图）](#情况二三通道常见的彩色图)
      - [情况三：四通道（CMYK 或 YCCK）](#情况三四通道cmyk-或-ycck)
    - [2. 其他解压缩参数的默认值](#2-其他解压缩参数的默认值)
  - [libjpeg 中的 YCCK -\> CMYK 转换](#libjpeg-中的-ycck---cmyk-转换)
  - [GraphicsMagick 中在读取 JPEG 图片单独对 CMYK 进行取反](#graphicsmagick-中在读取-jpeg-图片单独对-cmyk-进行取反)
  - [GraphicsMagick 示例代码](#graphicsmagick-示例代码)
  - [Little-CMS2 使用注意事项](#little-cms2-使用注意事项)
  - [其他](#其他)

## Adobe APP14 segment

```Bash
exiftool -b -Adobe ./启新京蔚高速徒步活动.jpg | hexdump -C
```

```
00000000  41 64 6f 62 65 00 64 40  00 00 00 02              |Adobe.d@....|
```

逐字节解释如下：

- `41 64 6f 62 65` → ASCII 字符串 `"Adobe"`，标识这是 Adobe 插入的 APP14 段。
- `00` → 一个空字节，用来分隔标识符和后续数据。
- `64 40` → 十六进制数值，通常表示 **版本号或标志位**。
- `00 00 00 02` → 这是 Adobe APP14 的 **颜色变换标志 (Color Transform)**。
    - `0` = 未指定
    - `1` = YCbCr
    - `2` = YCCK

## libjpeg 解码器对 APP14 的处理

待考证:

```
According to ISO/IEC 10918-6:2013, the interpretation of color space depends on:
    The number of components (channels)
    The presence of APP14 (Adobe) marker
    The transform flag inside APP14
```

```C
/*
 * Set default decompression parameters.
 */

LOCAL(void)
default_decompress_parms(j_decompress_ptr cinfo)
{
  /* Guess the input colorspace, and set output colorspace accordingly. */
  /* (Wish JPEG committee had provided a real way to specify this...) */
  /* Note application may override our guesses. */
  switch (cinfo->num_components) {
  case 1:
    cinfo->jpeg_color_space = JCS_GRAYSCALE;
    cinfo->out_color_space = JCS_GRAYSCALE;
    break;

  case 3:
    if (cinfo->saw_JFIF_marker) {
      cinfo->jpeg_color_space = JCS_YCbCr; /* JFIF implies YCbCr */
    } else if (cinfo->saw_Adobe_marker) {
      switch (cinfo->Adobe_transform) {
      case 0:
        cinfo->jpeg_color_space = JCS_RGB;
        break;
      case 1:
        cinfo->jpeg_color_space = JCS_YCbCr;
        break;
      default:
        WARNMS1(cinfo, JWRN_ADOBE_XFORM, cinfo->Adobe_transform);
        cinfo->jpeg_color_space = JCS_YCbCr; /* assume it's YCbCr */
        break;
      }
    } else {
      /* Saw no special markers, try to guess from the component IDs */
      int cid0 = cinfo->comp_info[0].component_id;
      int cid1 = cinfo->comp_info[1].component_id;
      int cid2 = cinfo->comp_info[2].component_id;

      if (cid0 == 1 && cid1 == 2 && cid2 == 3) {
#ifdef D_LOSSLESS_SUPPORTED
        if (cinfo->master->lossless)
          cinfo->jpeg_color_space = JCS_RGB; /* assume RGB w/out marker */
        else
#endif
          cinfo->jpeg_color_space = JCS_YCbCr; /* assume JFIF w/out marker */
      } else if (cid0 == 82 && cid1 == 71 && cid2 == 66)
        cinfo->jpeg_color_space = JCS_RGB; /* ASCII 'R', 'G', 'B' */
      else {
        TRACEMS3(cinfo, 1, JTRC_UNKNOWN_IDS, cid0, cid1, cid2);
#ifdef D_LOSSLESS_SUPPORTED
        if (cinfo->master->lossless)
          cinfo->jpeg_color_space = JCS_RGB; /* assume it's RGB */
        else
#endif
          cinfo->jpeg_color_space = JCS_YCbCr; /* assume it's YCbCr */
      }
    }
    /* Always guess RGB is proper output colorspace. */
    cinfo->out_color_space = JCS_RGB;
    break;

  case 4:
    if (cinfo->saw_Adobe_marker) {
      switch (cinfo->Adobe_transform) {
      case 0:
        cinfo->jpeg_color_space = JCS_CMYK;
        break;
      case 2:
        cinfo->jpeg_color_space = JCS_YCCK;
        break;
      default:
        WARNMS1(cinfo, JWRN_ADOBE_XFORM, cinfo->Adobe_transform);
        cinfo->jpeg_color_space = JCS_YCCK; /* assume it's YCCK */
        break;
      }
    } else {
      /* No special markers, assume straight CMYK. */
      cinfo->jpeg_color_space = JCS_CMYK;
    }
    cinfo->out_color_space = JCS_CMYK;
    break;

  default:
    cinfo->jpeg_color_space = JCS_UNKNOWN;
    cinfo->out_color_space = JCS_UNKNOWN;
    break;
  }

  /* Set defaults for other decompression parameters. */
  cinfo->scale_num = 1;         /* 1:1 scaling */
  cinfo->scale_denom = 1;
  cinfo->output_gamma = 1.0;
  cinfo->buffered_image = FALSE;
  cinfo->raw_data_out = FALSE;
  cinfo->dct_method = JDCT_DEFAULT;
  cinfo->do_fancy_upsampling = TRUE;
  cinfo->do_block_smoothing = TRUE;
  cinfo->quantize_colors = FALSE;
  /* We set these in case application only sets quantize_colors. */
  cinfo->dither_mode = JDITHER_FS;
#ifdef QUANT_2PASS_SUPPORTED
  cinfo->two_pass_quantize = TRUE;
#else
  cinfo->two_pass_quantize = FALSE;
#endif
  cinfo->desired_number_of_colors = 256;
  cinfo->colormap = NULL;
  /* Initialize for no mode change in buffered-image mode. */
  cinfo->enable_1pass_quant = FALSE;
  cinfo->enable_external_quant = FALSE;
  cinfo->enable_2pass_quant = FALSE;
}
```

### 1. 输入与输出颜色空间的推断

代码首先根据 cinfo->num_components（JPEG 图像的分量数）来推断输入颜色空间 (jpeg_color_space) 和输出颜色空间 (out_color_space)。

#### 情况一：单通道（灰度图）

- 如果只有一个分量，直接认为是灰度图。

#### 情况二：三通道（常见的彩色图）

- 如果有 JFIF 标记 → 输入颜色空间设为 YCbCr。
- 如果有 Adobe 标记 → 根据 Adobe_transform 决定：
    - 0 → RGB
    - 1 → YCbCr
    - 其他值 → 警告并假设 YCbCr。
- 如果没有特殊标记 → 根据分量 ID 猜测：
    - 如果是 1,2,3 → 默认 YCbCr（但在无损模式下可能是 RGB）。
    - 如果是 ASCII 'R','G','B' → RGB。
    - 否则 → 打印 trace 信息，假设 YCbCr（无损模式下假设 RGB）
- 输出颜色空间始终设为 RGB（因为应用程序通常希望得到 RGB）。

#### 情况三：四通道（CMYK 或 YCCK）

- 有 Adobe 标记时，根据 Adobe_transform 判断是 CMYK 还是 YCCK。
    (`saw_Adobe_marker` 在`examine_app14`函数中被设置)
- **没有标记时，默认认为是 CMYK**。
- 输出颜色空间设为 CMYK。

### 2. 其他解压缩参数的默认值

在颜色空间设置完成后，代码还初始化了一些解码参数：

- 缩放比例：scale_num = 1, scale_denom = 1 → 不缩放。
- Gamma：output_gamma = 1.0 → 不做 gamma 校正。
- 缓冲模式：buffered_image = FALSE → 不启用缓冲图像模式。
- 原始数据输出：raw_data_out = FALSE → 默认输出处理后的像素。
- DCT 方法：dct_method = JDCT_DEFAULT → 使用默认的离散余弦变换方法。
- 插值与平滑：
    - do_fancy_upsampling = TRUE → 启用高质量上采样。
    - do_block_smoothing = TRUE → 启用块平滑。
- 颜色量化：
    - quantize_colors = FALSE → 默认不量化颜色。
    - dither_mode = JDITHER_FS → 抖动模式设为 Floyd-Steinberg。
    - two_pass_quantize → 如果支持两遍量化则启用，否则禁用。
    - desired_number_of_colors = 256 → 默认目标颜色数。
    - colormap = NULL → 没有预设的颜色映射表。
- 缓冲图像模式下的量化选项：
    - enable_1pass_quant = FALSE
    - enable_external_quant = FALSE
    - enable_2pass_quant = FALSE

## libjpeg 中的 YCCK -> CMYK 转换

```C
/*
 * Adobe-style YCCK->CMYK conversion.
 * We convert YCbCr to R=1-C, G=1-M, and B=1-Y using the same
 * conversion as above, while passing K (black) unchanged.
 * We assume build_ycc_rgb_table has been called.
 */

METHODDEF(void)
ycck_cmyk_convert(j_decompress_ptr cinfo, _JSAMPIMAGE input_buf,
                  JDIMENSION input_row, _JSAMPARRAY output_buf, int num_rows)
{
#if BITS_IN_JSAMPLE != 16
  my_cconvert_ptr cconvert = (my_cconvert_ptr)cinfo->cconvert;
  register int y, cb, cr;
  register _JSAMPROW outptr;
  register _JSAMPROW inptr0, inptr1, inptr2, inptr3;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;
  /* copy these pointers into registers if possible */
  register _JSAMPLE *range_limit = (_JSAMPLE *)cinfo->sample_range_limit;
  register int *Crrtab = cconvert->Cr_r_tab;
  register int *Cbbtab = cconvert->Cb_b_tab;
  register JLONG *Crgtab = cconvert->Cr_g_tab;
  register JLONG *Cbgtab = cconvert->Cb_g_tab;
  SHIFT_TEMPS

  while (--num_rows >= 0) {
    inptr0 = input_buf[0][input_row];
    inptr1 = input_buf[1][input_row];
    inptr2 = input_buf[2][input_row];
    inptr3 = input_buf[3][input_row];
    input_row++;
    outptr = *output_buf++;
    for (col = 0; col < num_cols; col++) {
      y  = inptr0[col];
      cb = inptr1[col];
      cr = inptr2[col];
      /* Range-limiting is essential due to noise introduced by DCT losses. */
      outptr[0] = range_limit[_MAXJSAMPLE - (y + Crrtab[cr])];  /* red */
      outptr[1] = range_limit[_MAXJSAMPLE - (y +                /* green */
                              ((int)RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
                                                 SCALEBITS)))];
      outptr[2] = range_limit[_MAXJSAMPLE - (y + Cbbtab[cb])];  /* blue */
      /* K passes through unchanged */
      outptr[3] = inptr3[col];
      outptr += 4;
    }
  }
#else
  ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
#endif
}
```

这是一个 Adobe 风格的 YCCK → CMYK 转换函数。

- 输入：YCCK 图像数据（Y、Cb、Cr、K 四通道）。
- 输出：CMYK 图像数据。
- 原理：先将 YCbCr 转换为 RGB，再通过 R=1-C, G=1-M, B=1-Y 的关系得到 CMY，同时保持 K 通道不变。

函数签名

- cinfo：JPEG 解压缩上下文，包含颜色转换表、采样范围等信息。
- input_buf：输入图像缓冲区，四个通道（Y、Cb、Cr、K）。
- input_row：当前处理的行索引。
- output_buf：输出缓冲区（CMYK）。
- num_rows：要处理的行数。

关键变量

- inptr0..3：分别指向 Y、Cb、Cr、K 通道的输入行。
- outptr：输出行指针。
- range_limit：范围限制表，用于避免 DCT 损失导致的溢出。
- Crrtab, Cbbtab, Crgtab, Cbgtab：预计算的查找表，用于快速 YCbCr → RGB 转换。

条件编译

- 如果采样精度不是 16 位（通常是 8 位），执行上述转换。否则报错，说明未实现 16 位采样的转换。

```
输入: (Y, Cb, Cr, K)
   ↓ YCbCr → RGB
R = Y + 1.402*(Cr-128)
G = Y - 0.344*(Cb-128) - 0.714*(Cr-128)
B = Y + 1.772*(Cb-128)
   ↓ RGB → CMY
C = 255 - R
M = 255 - G
Y = 255 - B
K = K
   ↓ 裁剪
输出: (C, M, Y, K)
```

## GraphicsMagick 中在读取 JPEG 图片单独对 CMYK 进行取反

```C++
static Image* ReadJPEGImage(const ImageInfo* image_info, ExceptionInfo* exception)
{
    // ...

    /*
      Convert JPEG pixels to pixel packets.
    */
    for (y = 0; y < (long)image->rows; y++)
    {
        // ...
        if (jpeg_info.output_components == 1)
        {
            // ...
        }
        else if ((jpeg_info.output_components == 3) || (jpeg_info.output_components == 4))
        {
            // ...
            if (image->colorspace == CMYKColorspace)
            {
                /*
                  CMYK pixels are inverted.
                */
                q = AccessMutablePixels(image);
                for (x = 0; x < (long)image->columns; x++)
                {
                    q->red = MaxRGB - q->red;
                    q->green = MaxRGB - q->green;
                    q->blue = MaxRGB - q->blue;
                    q->opacity = MaxRGB - q->opacity;
                    q++;
                }
            }
        }
        // ...
    }

    // ...
}
```

```C++
#if (QuantumDepth == 8)
#  define MaxRGB  255U
#elif (QuantumDepth == 16)
#  define MaxRGB  65535U
#elif (QuantumDepth == 32)
#  define MaxRGB  4294967295U
#else
#  error "Specified value of QuantumDepth is not supported"
#endif
```

## GraphicsMagick 示例代码

``` Sh
gm convert $HOME/Pictures/cmyk.jpg -profile "$HOME/Pictures/ICC Profiles/sRGBz.icc" out.png
```

``` C++
std::string to_string(Magick::ColorspaceType colorspace) {
    switch (colorspace) {
        case Magick::CMYKColorspace:        return "CMYKColorspace";
        case Magick::GRAYColorspace:        return "GRAYColorspace";
        case Magick::HSLColorspace:         return "HSLColorspace";
        case Magick::HWBColorspace:         return "HWBColorspace";
        case Magick::OHTAColorspace:        return "OHTAColorspace";
        case Magick::Rec601YCbCrColorspace: return "Rec601YCbCrColorspace";
        case Magick::Rec709YCbCrColorspace: return "Rec709YCbCrColorspace";
        case Magick::RGBColorspace:         return "RGBColorspace";
        case Magick::sRGBColorspace:        return "sRGBColorspace";
        case Magick::TransparentColorspace: return "TransparentColorspace";
        case Magick::YCCColorspace:         return "YCCColorspace";
        case Magick::YIQColorspace:         return "YIQColorspace";
        case Magick::YPbPrColorspace:       return "YPbPrColorspace";
        case Magick::YUVColorspace:         return "YUVColorspace";
        case Magick::UndefinedColorspace:   return "UndefinedColorspace";
        default:                            return "UndefinedColorspace";
    }
}

int main(int argc, char** argv) {
    auto jpeg_path = utility::win32::mypictures_directory_path() / L"cmyk.jpg";
    auto jpeg_path_str = utility::to_string(jpeg_path.generic_wstring());
    auto icc_profile_path = utility::win32::mypictures_directory_path() / L"ICC Profiles" / L"sRGBz.icc";
    std::ifstream file(icc_profile_path, std::ios::binary);
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

    Magick::InitializeMagick(*argv);
    try {
        Magick::Image image(jpeg_path_str);
        std::cout << " Format: " << image.magick() << '\n';
        std::cout << " Width: " << image.columns() << " pixels" << '\n';
        std::cout << " Height: " << image.rows() << " pixels" << '\n';
        std::cout << " Depth: " << image.depth() << " bits" << '\n';
        std::cout << " ColorSpace (before conversion): " << to_string(image.colorSpace()) << '\n';
        Magick::Blob icc_profile_blob(buffer.data(), buffer.size());
        image.profile("ICM", icc_profile_blob);
        std::cout << " ColorSpace (after conversion): " << to_string(image.colorSpace()) << '\n';
        image.write("output.png");
    } catch (Magick::Exception &error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

## Little-CMS2 使用注意事项

1. 单像素转换结果都没有问题，如 TYPE_CMYK_8 -> TYPE_RGB_8，TYPE_CMYK_8 -> TYPE_RGBA_8；
2. 批量像素转换在通道数不同时候会错位，如 TYPE_CMYK_8 -> TYPE_RGB_8；
3. 批量像素转换 TYPE_CMYK_8 -> TYPE_RGBA_8 时，cms 给出的 RGBA 像素 Alpha 通道是 0，需要手动置为 255；
4. ImageMagick 使用 cmsDoTransformTHR 多线程版本；
5. GraphicMagick 使用 cmsDoTransform 转换单个像素


## 其他

|| ImageMagick | GraphicsMagick |
|---|---|---|
| DB 库和 RL 库是否通用 | 否，用 RL 和 DB 前缀区分 | 否，用 RL 和 DB 前缀区分 |
| 官方是否提供 RL 版开发库 | 是 | 否，需要自己编译 |
| 官方是否提供 DB 版开发库 | 否，需要自己编译 | 否，需要自己编译 |
| DLL 大小 | < 9M | < 9M |
| Q8 版 Release 转换速度 | 0.8-0.9s | 0.4-0.5s |
