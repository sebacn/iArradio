
#include "JPEGDEC.h"
#include "FS.h"
#include "SD.h"
#include "locallog.hpp"
#include "imagerender.hpp"

JPEGDEC jpeg;

struct Image {
    uint16_t w;
    uint16_t h;
    uint8_t *Buff;
};

Image img;
Image jpg;
uint16_t jpg_ptr = 0;
File myFile;

//extern GxEPD2_4G_4G<GxEPD2_270, GxEPD2_270::HEIGHT> display;

uint8_t get_point(uint8_t *p, uint16_t rows, uint16_t cols, uint16_t x, uint16_t y);
void set_point(uint8_t *p, uint16_t rows, uint16_t cols, uint16_t x, uint16_t y, float f);
void get_adjacents_2d(uint8_t *src, float *dest, uint16_t rows, uint16_t cols, uint16_t x, uint16_t y);
float cubicInterpolate(float p[], float x);
float bicubicInterpolate(float p[], float x, float y);
void interpolate_image(uint8_t *src, uint16_t src_rows, uint16_t src_cols,
                       uint8_t *dest, uint16_t dest_rows, uint16_t dest_cols);

/*---------------------------------------------------------
 * Function: get_point
 * Inputs:
 *      int8_t *p:
 *      uint8_t rows:
 *      uint8_t cols:
 *      uint8_t x:
 *      uint8_t y:
 * Outputs:
 *      uint8_t:
 * Description:
 ---------------------------------------------------------*/
uint8_t get_point(uint8_t *p, uint16_t rows, uint16_t cols, uint16_t x, uint16_t y) {
  if (x < 0)        x = 0;
  if (y < 0)        y = 0;
  if (x >= cols)    x = cols - 1;
  if (y >= rows)    y = rows - 1;
  return p[y * cols + x];
}

/*---------------------------------------------------------
 * Function: set_point
 * Inputs:
 *      uint8_t *p:
 *      uint8_t rows:
 *      uint8_t cols:
 *      int8_t x:
 *      int8_t y:
 *      float f:
 * Outputs:
 *      void (none)
 * Description:
 ---------------------------------------------------------*/
void set_point(uint8_t *p, uint16_t rows, uint16_t cols, uint16_t x, uint16_t y, float f) {
  if ((x < 0) || (x >= cols)) return;
  if ((y < 0) || (y >= rows)) return;
  p[y * cols + x] = (uint8_t)f;
}


/*---------------------------------------------------------
 * Function: interpolate_image
 * Inputs:
 *      int8_t *src:
 *      uint8_t src_rows:
 *      uint8_t src_cols:
 *      uint8_t *dest:
 *      uint8_t dest_rows:
 *      uint8_t dest_cols:
 * Outputs:
 *      void (none)
 * Description:
 ---------------------------------------------------------*/
// src is a grid src_rows * src_cols
// dest is a pre-allocated grid, dest_rows*dest_cols
void interpolate_image(uint8_t *src, uint16_t src_rows, uint16_t src_cols, 
                       uint8_t *dest, uint16_t dest_rows, uint16_t dest_cols) {
  float mu_x = (src_cols - 1.0) / (dest_cols - 1.0);
  float mu_y = (src_rows - 1.0) / (dest_rows - 1.0);

  float adj_2d[16]; // matrix for storing adjacents
  
  for (uint8_t y_idx=0; y_idx < dest_rows; y_idx++) {
    for (uint8_t x_idx=0; x_idx < dest_cols; x_idx++) {
       float x = x_idx * mu_x;
       float y = y_idx * mu_y;
       //Serial.print("("); Serial.print(y_idx); Serial.print(", "); Serial.print(x_idx); Serial.print(") = ");
       //Serial.print("("); Serial.print(y); Serial.print(", "); Serial.print(x); Serial.print(") = ");
       get_adjacents_2d(src, adj_2d, src_rows, src_cols, x, y);
       
       //Serial.print("[");
       //for (uint8_t i=0; i<16; i++) {
         //Serial.print(adj_2d[i]); Serial.print(", ");
       //}
       //Serial.println("]");
       
       float frac_x = x - (int)x; // we only need the ~delta~ between the points
       float frac_y = y - (int)y; // we only need the ~delta~ between the points
       float out = bicubicInterpolate(adj_2d, frac_x, frac_y);
       //Serial.print("\tInterp: "); Serial.println(out);
       set_point(dest, dest_rows, dest_cols, x_idx, y_idx, out);
    }
  }
}

/*---------------------------------------------------------
 * Function: cubicInterpolate
 * Inputs:
 *      float p[]:
 *      float x:
 * Outputs:
 *      float:
 * Description:
 ---------------------------------------------------------*/
// p is a list of 4 points, 2 to the left, 2 to the right
float cubicInterpolate(float p[], float x) {
    float r = p[1] + (0.5 * x * (p[2] - p[0] + x*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + x*(3.0*(p[1] - p[2]) + p[3] - p[0]))));
  /*
    Serial.print("interpolating: ["); 
    Serial.print(p[0],2); Serial.print(", ");
    Serial.print(p[1],2); Serial.print(", ");
    Serial.print(p[2],2); Serial.print(", ");
    Serial.print(p[3],2); Serial.print("] w/"); Serial.print(x); Serial.print(" = ");
    Serial.println(r);
  */
    return r;
}

/*---------------------------------------------------------
 * Function: bicubicInterpolate
 * Inputs:
 *      float p[]:
 *      float x:
 *      float y:
 * Outputs:
 *      float:
 * Description:
 ---------------------------------------------------------*/
// p is a 16-point 4x4 array of the 2 rows & columns left/right/above/below
float bicubicInterpolate(float p[], float x, float y) {
    float arr[4] = {0,0,0,0};
    arr[0] = cubicInterpolate(p+0, x);
    arr[1] = cubicInterpolate(p+4, x);
    arr[2] = cubicInterpolate(p+8, x);
    arr[3] = cubicInterpolate(p+12, x);
    return cubicInterpolate(arr, y);
}

/*---------------------------------------------------------
 * Function: get_adjacent_2d
 * Inputs:
 *      int8_t *src:
 *      float *dest:
 *      uint8_t rows:
 *      uint8_t cols:
 *      int8_t x:
 *      int8_t y:
 * Outputs:
 *      void (none)
 * Description:
 ---------------------------------------------------------*/
// src is rows*cols and dest is a 16-point array passed in already allocated!
void get_adjacents_2d(uint8_t *src, float *dest, uint16_t rows, uint16_t cols, uint16_t x, uint16_t y) {
    //Serial.print("("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.println(")");
    float arr[4];
    for (int8_t delta_y = -1; delta_y < 3; delta_y++) { // -1, 0, 1, 2
        float *row = dest + 4 * (delta_y+1); // index into each chunk of 4
        for (int8_t delta_x = -1; delta_x < 3; delta_x++) { // -1, 0, 1, 2
            row[delta_x+1] = get_point(src, rows, cols, x+delta_x, y+delta_y);
        }
    }
}

int JPEGDraw(JPEGDRAW *pDraw)
{
    //display.drawGreyPixmap((uint8_t *) pDraw->pPixels, 2, pDraw->x+10, pDraw->y, pDraw->iWidth, pDraw->iHeight);
    //display.nextPage();

    uint16_t lenn = pDraw->iWidth * pDraw->iHeight;

    //lenn = lenn >> 2; /4 for 2bpp

    //llog_d("DRAW: x %d, y %d, iWidth %d, iHeight %d, jpg_ptr %d, lenn %d", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, jpg_ptr, lenn);

    memcpy(jpg.Buff + jpg_ptr, pDraw->pPixels, lenn);

    jpg_ptr = jpg_ptr + lenn;

    jpg.w = pDraw->x + pDraw->iWidth;
    jpg.h = pDraw->y + pDraw->iHeight;

    return 1;
}

void render4GJpegFile(const char *filename, GxEPD2_4G_4G<GxEPD2_270, GxEPD2_270::HEIGHT> * display, uint16_t x, uint16_t y, uint16_t img_w, uint16_t img_h)
{
    bool err = false;    

    llog_i("render4GJpegFile %s start", filename);

    img.Buff = (uint8_t *)ps_calloc((uint16_t) img_w*img_h + 10, sizeof(uint8_t));

    if (img.Buff == NULL) {
        llog_e("Initial alloc imgBuff failed!");
        return;
    }

    llog_i("img.Buff %dx%d, size %d!", img_w, img_h, img_w*img_h + 10);

    myFile = SD.open(filename, FILE_READ);

    if (jpeg.open(myFile, JPEGDraw)) { //openRAM(source_buf, img_buf_pos, JPEGDraw4Bits)) {

        llog_i("JPEG size: %d x %d, orientation: %d, bpp: %d", jpeg.getWidth(), jpeg.getHeight(), jpeg.getOrientation(), jpeg.getBpp());

        if (jpeg.hasThumb())
        {
            llog_i("Thumbnail present: %d x %d", jpeg.getThumbWidth(), jpeg.getThumbHeight());
        }       

        jpeg.setPixelType(EIGHT_BIT_GRAYSCALE); // TWO_BIT_DITHERED, FOUR_BIT_DITHERED

        int jw = jpeg.getWidth();
        int jh = jpeg.getHeight();
        int jscale = 1;

        llog_i("JPEG scale w:%d->%d, h:%d->%d scale %d", jw, img_w, jh, img_h, jscale);

        while (jscale < 8 && (jw/2 >= img_w || jh/2 >= img_h))
        {           
            jw /= 2;
            jh /= 2;
            jscale <<= 1;                     
            llog_i("JPEG scale %dx%d, scale %d", jw, jh, jscale);
        }

        uint16_t jpgBufSize = (uint16_t)(jw+4)*jh;

        llog_i("JPEG Buff %dx%d, scale %d, size %d!", jw+4, jh, jscale, jpgBufSize);

        jpg.Buff = (uint8_t *)ps_calloc(jpgBufSize, sizeof(uint8_t)); //additional 4 bytes for each row

        jpg_ptr = 0;

        if (jpg.Buff == NULL) {
            llog_e("JPEG alloc jpgBuff failed!");
            err = true;
        }
        else if (jpeg.decode(0, 0, jscale)) //JPEG_SCALE_HALF))
        {
            llog_d("JPEG decode Ok - %dx%d", jpg.w, jpg.h);
        } else {
            llog_e("JPEG decode Failed with error: %d", jpeg.getLastError());
            err = true;
        }
    } 
    else 
    {
        llog_e("jpeg.open Failed with error: %d", jpeg.getLastError());
        err = true;
    }

    jpeg.close();

    if (err)
    {
        llog_i("Error: free buff & exit");
        free(jpg.Buff);
        free(img.Buff);
        return;
    }

    if (jpg.w != img_w || jpg.h != img_h)
    {
        llog_e("Interpolate image from %dx%d to %dx%d", jpg.w, jpg.h, img_w, img_h);
        interpolate_image(jpg.Buff, jpg.h, jpg.w, img.Buff, img_h, img_w);
        //memcpy(img.Buff, jpg.Buff, img_w*img_h);
    }
    else
    {
        memcpy(img.Buff, jpg.Buff, img_w*img_h);
    }

    llog_i("Draw image");
    //display->firstPage();
    do
    {
      
        llog_i("drawGreyPixma x:%d,y:%d %dx%d", x, y, img_w, img_h);
        display->drawGreyPixmap((uint8_t *) img.Buff, 8, x, y, img_w, img_h);
    }
    while (display->nextPage());

    display->powerOff();

    llog_i("free jpg buff");
    free(jpg.Buff);
    llog_i("free img buff");
    free(img.Buff);

    myFile.close();

    llog_i("render4GJpegFile %s end\n", filename);
}

/*
uint16_t read16(File& f)
{
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File& f)
{
  // BMP data is stored little-endian, same as Arduino.
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void readBitmapFromSD(const char *filename, uint8_t *imgBuff, uint16_t img_w, uint16_t img_h) //int16_t x, int16_t y, bool with_color)
{
  File file;

//static const uint16_t input_buffer_pixels = 20; // may affect performance
static const uint16_t input_buffer_pixels = 800; // may affect performance

static const uint16_t max_row_width = 1448; // for up to 6" display 1448x1072
static const uint16_t max_palette_pixels = 256; // for depth <= 8

uint8_t input_buffer[3 * input_buffer_pixels]; // up to depth 24
uint8_t output_row_mono_buffer[max_row_width / 8]; // buffer for at least one row of b/w bits
uint8_t output_row_color_buffer[max_row_width / 8]; // buffer for at least one row of color bits
uint8_t mono_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 b/w
uint8_t color_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 c/w
uint16_t rgb_palette_buffer[max_palette_pixels]; // palette buffer for depth <= 8 for buffered graphics, needed for 7-color display

  bool with_color = false;
  bool valid = false; // valid format to be handled
  bool flip = true; // bitmap is stored bottom-to-top
  uint32_t startTime = millis();
  //if ((x >= display.epd2.WIDTH) || (y >= display.epd2.HEIGHT)) return;
  Serial.println();
  Serial.print("Loading image '");
  Serial.print(filename);
  Serial.println('\'');
#if defined(ESP32)
  file = SD.open(String("/") + filename, FILE_READ);
  if (!file)
  {
    Serial.print("File not found");
    return;
  }
#else
  file = SD.open(filename);
  if (!file)
  {
    Serial.print("File not found");
    return;
  }
#endif
  // Parse BMP header
  if (read16(file) == 0x4D42) // BMP signature
  {
    uint32_t fileSize = read32(file);
    uint32_t creatorBytes = read32(file); (void)creatorBytes; //unused
    uint32_t imageOffset = read32(file); // Start of image data
    uint32_t headerSize = read32(file);
    uint32_t width  = read32(file);
    int32_t height = (int32_t) read32(file);
    uint16_t planes = read16(file);
    uint16_t depth = read16(file); // bits per pixel
    uint32_t format = read32(file);
    if ((planes == 1) && ((format == 0) || (format == 3))) // uncompressed is handled, 565 also
    {
      Serial.print("File size: "); Serial.println(fileSize);
      Serial.print("Image Offset: "); Serial.println(imageOffset);
      Serial.print("Header size: "); Serial.println(headerSize);
      Serial.print("Bit Depth: "); Serial.println(depth);
      Serial.print("Image size: ");
      Serial.print(width);
      Serial.print('x');
      Serial.println(height);
      // BMP rows are padded (if needed) to 4-byte boundary
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (depth < 8) rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
      if (height < 0)
      {
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;
      if ((x + w - 1) >= display.epd2.WIDTH)  w = display.epd2.WIDTH  - x;
      if ((y + h - 1) >= display.epd2.HEIGHT) h = display.epd2.HEIGHT - y;
      if (w <= max_row_width) // handle with direct drawing
      {
        valid = true;
        uint8_t bitmask = 0xFF;
        uint8_t bitshift = 8 - depth;
        uint16_t red, green, blue;
        bool whitish = false;
        bool colored = false;
        if (depth == 1) with_color = false;
        if (depth <= 8)
        {
          if (depth < 8) bitmask >>= depth;
          //file.seek(54); //palette is always @ 54
          file.seek(imageOffset - (4 << depth)); // 54 for regular, diff for colorsimportant
          for (uint16_t pn = 0; pn < (1 << depth); pn++)
          {
            blue  = file.read();
            green = file.read();
            red   = file.read();
            file.read();
            //Serial.print(red); Serial.print(" "); Serial.print(green); Serial.print(" "); Serial.println(blue);
            whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
            colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
            if (0 == pn % 8) mono_palette_buffer[pn / 8] = 0;
            mono_palette_buffer[pn / 8] |= whitish << pn % 8;
            if (0 == pn % 8) color_palette_buffer[pn / 8] = 0;
            color_palette_buffer[pn / 8] |= colored << pn % 8;
          }
        }
        display.clearScreen();
        uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;
        for (uint16_t row = 0; row < h; row++, rowPosition += rowSize) // for each line
        {
          uint32_t in_remain = rowSize;
          uint32_t in_idx = 0;
          uint32_t in_bytes = 0;
          uint8_t in_byte = 0; // for depth <= 8
          uint8_t in_bits = 0; // for depth <= 8
          uint8_t out_byte = 0xFF; // white (for w%8!=0 border)
          uint8_t out_color_byte = 0xFF; // white (for w%8!=0 border)
          uint32_t out_idx = 0;
          file.seek(rowPosition);
          for (uint16_t col = 0; col < w; col++) // for each pixel
          {
            // Time to read more pixel data?
            if (in_idx >= in_bytes) // ok, exact match for 24bit also (size IS multiple of 3)
            {
              in_bytes = file.read(input_buffer, in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain);
              in_remain -= in_bytes;
              in_idx = 0;
            }
            switch (depth)
            {
              case 32:
                blue = input_buffer[in_idx++];
                green = input_buffer[in_idx++];
                red = input_buffer[in_idx++];
                in_idx++; // skip alpha
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
                break;
              case 24:
                blue = input_buffer[in_idx++];
                green = input_buffer[in_idx++];
                red = input_buffer[in_idx++];
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
                break;
              case 16:
                {
                  uint8_t lsb = input_buffer[in_idx++];
                  uint8_t msb = input_buffer[in_idx++];
                  if (format == 0) // 555
                  {
                    blue  = (lsb & 0x1F) << 3;
                    green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                    red   = (msb & 0x7C) << 1;
                  }
                  else // 565
                  {
                    blue  = (lsb & 0x1F) << 3;
                    green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                    red   = (msb & 0xF8);
                  }
                  whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                  colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
                }
                break;
              case 1:
              case 2:
              case 4:
              case 8:
                {
                  if (0 == in_bits)
                  {
                    in_byte = input_buffer[in_idx++];
                    in_bits = 8;
                  }
                  uint16_t pn = (in_byte >> bitshift) & bitmask;
                  whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
                  colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
                  in_byte <<= depth;
                  in_bits -= depth;
                }
                break;
            }
            if (whitish)
            {
              // keep white
            }
            else if (colored && with_color)
            {
              out_color_byte &= ~(0x80 >> col % 8); // colored
            }
            else
            {
              out_byte &= ~(0x80 >> col % 8); // black
            }
            if ((7 == col % 8) || (col == w - 1)) // write that last byte! (for w%8!=0 border)
            {
              output_row_color_buffer[out_idx] = out_color_byte;
              output_row_mono_buffer[out_idx++] = out_byte;
              out_byte = 0xFF; // white (for w%8!=0 border)
              out_color_byte = 0xFF; // white (for w%8!=0 border)
            }
          } // end pixel
          uint16_t yrow = y + (flip ? h - row - 1 : row);
          //display.writeImage(output_row_mono_buffer, output_row_color_buffer, x, yrow, w, 1);
          display.writeImage_4G(output_row_mono_buffer, depth, x, yrow, w, 1);
        } // end line
        Serial.print("loaded in "); Serial.print(millis() - startTime); Serial.println(" ms");
        display.refresh();
      }
    }
  }
  file.close();
  if (!valid)
  {
    Serial.println("bitmap format not handled.");
  }
}

*/