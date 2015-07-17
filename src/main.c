#include <pebble.h>
  
// Comvience Macros
#ifdef PBL_COLOR
  #define IF_COLOR(statement) (statement)
  #define IF_BW(statement)
  #define IF_COLORBW(color, bw) (color)
  #define IF_BWCOLOR(bw, color) (color)
#else
  #define IF_COLOR(statement)
  #define IF_BW(statement) (statement)
  #define IF_COLORBW(color, bw) (bw)
  #define IF_BWCOLOR(bw, color) (bw)
#endif
  
Window *main_window;
Layer *root_layer, *graphics_layer;

GBitmap **image;
uint8_t current_image=0;
uint8_t number_of_images=0;
uint16_t power(uint16_t x, uint16_t y) {uint16_t r=1; for(uint8_t i=0; i<y; ++i) r*=x; return r;} // Super stupid power function, for demonstration purposes only!

// ================================================================ //
//   How to support transparencies and the alpha channel
// ================================================================ //
uint8_t shadowtable[] = {192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,  /* ------------------ */ \
                         192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,  /*      0% alpha      */ \
                         192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,  /*        Clear       */ \
                         192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,  /* ------------------ */ \

                         192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197,  /* ------------------ */ \
                         192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197,  /*     33% alpha      */ \
                         192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197,  /*    Transparent     */ \
                         208,208,208,209,208,208,208,209,208,208,208,209,212,212,212,213,  /* ------------------ */ \

                         192,192,193,194,192,192,193,194,196,196,197,198,200,200,201,202,  /* ------------------ */ \
                         192,192,193,194,192,192,193,194,196,196,197,198,200,200,201,202,  /*     66% alpha      */ \
                         208,208,209,210,208,208,209,210,212,212,213,214,216,216,217,218,  /*    Translucent     */ \
                         224,224,225,226,224,224,225,226,228,228,229,230,232,232,233,234,  /* ------------------ */ \

                         192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,  /* ------------------ */ \
                         208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,  /*    100% alpha      */ \
                         224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,  /*      Opaque        */ \
                         240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255}; /* ------------------ */

uint8_t combine_colors(uint8_t bg_color, uint8_t fg_color) {
  return (shadowtable[((~fg_color)&0b11000000) + (bg_color&63)]&63) + shadowtable[fg_color];
}

// Fill a rectangular region with a possibly-translucent color -- Only works on Color
void fill_rect(GContext *ctx, GRect rect, uint8_t color) {
  #ifdef PBL_COLOR
  // Bounds Checking -- feel free to remove this section if you know you won't go out of bounds
  rect.size.h = (rect.size.h + rect.origin.y) > 168 ? 168 - rect.origin.y : rect.size.h;
  rect.size.w = (rect.size.w + rect.origin.x) > 144 ? 144 - rect.origin.x : rect.size.w;
  if(rect.origin.y < 0) {rect.size.h += rect.origin.y; rect.origin.y = 0;}
  if(rect.origin.x < 0) {rect.size.w += rect.origin.x; rect.origin.x = 0;}
  // End Bounds Checking
  
  GBitmap* framebuffer = graphics_capture_frame_buffer(ctx);
  if(framebuffer) {   // if successfully captured the framebuffer
    uint8_t* screen = gbitmap_get_data(framebuffer);
    for(uint16_t y_addr=rect.origin.y*144, row=0; row<rect.size.h; y_addr+=144, ++row)
      for(uint16_t x_addr=rect.origin.x, x=0; x<rect.size.w; ++x_addr, ++x)
        screen[y_addr+x_addr] = (shadowtable[((~color)&0b11000000) + (screen[y_addr+x_addr]&63)]&63) + shadowtable[color];
    graphics_release_frame_buffer(ctx, framebuffer);
  }
  #endif
}
// ================================================================ //
//  End Transparencies
// ================================================================ //









// get_gbitmapformat_text() lovingly stolen from:
//   https://github.com/rebootsramblings/GBitmap-Colour-Palette-Manipulator/blob/master/src/gbitmap_color_palette_manipulator.c
char* get_gbitmapformat_text(GBitmapFormat format) {
	switch (format) {
		case GBitmapFormat1Bit:        return "GBitmapFormat1Bit";
    #ifdef PBL_COLOR
		case GBitmapFormat8Bit:        return "GBitmapFormat8Bit";
		case GBitmapFormat1BitPalette: return "GBitmapFormat1BitPalette";
		case GBitmapFormat2BitPalette: return "GBitmapFormat2BitPalette";
		case GBitmapFormat4BitPalette: return "GBitmapFormat4BitPalette";
    #endif
		default:                       return "UNKNOWN FORMAT";
	}
}

uint8_t get_number_ofcolors(GBitmapFormat format) {
  switch (format) {
    case GBitmapFormat1Bit:        return  2;
    #ifdef PBL_COLOR
    case GBitmapFormat8Bit:        return 64;
    case GBitmapFormat1BitPalette: return  2;
    case GBitmapFormat2BitPalette: return  4;
    case GBitmapFormat4BitPalette: return 16;
    #endif
    default:                       return  0;
  }
}

void load_graphics() {
  const int image_resource[] = {
    RESOURCE_ID_TEST_32_1B,
    RESOURCE_ID_TEST_32_2B,
    RESOURCE_ID_TEST_32_4B,
    RESOURCE_ID_TEST_32_8B,
    RESOURCE_ID_TEST_64_1B,
    RESOURCE_ID_TEST_64_2B,
    RESOURCE_ID_TEST_64_4B,
    RESOURCE_ID_TEST_64_8B,
    RESOURCE_ID_FONT8,
    RESOURCE_ID_DOG
  };
  number_of_images = sizeof(image_resource)/sizeof(image_resource[0]);  // Number of images in image_resource[]
  image = malloc(sizeof(image) * number_of_images);                     // Allocate the array of pointers
 
  for(uint8_t i=0; i<number_of_images; ++i) {
    image[i] = NULL;  // Shouldn't be needed, but initializing array just incase the line below fails
    image[i] = gbitmap_create_with_resource(image_resource[i]); // Load the image (returns NULL if failed to load)
  }
}

void unload_graphics() {
   for(uint8_t i=0; i<number_of_images; ++i)
     if(image[i])
       gbitmap_destroy(image[i]);
  
  free(image);  // Free malloc'd image pointer array
}


void draw_image(GContext *ctx, GBitmap *image, int16_t start_x, int16_t start_y) {
  GBitmap *framebuffer = graphics_capture_frame_buffer(ctx);  // Get framebuffer
  if(framebuffer) {                                           // If successfully captured the framebuffer
    uint8_t        *screen = gbitmap_get_data(framebuffer);   // Get pointer to framebuffer data
    
    uint8_t          *data = (uint8_t*)gbitmap_get_data(image);
    int16_t          width = gbitmap_get_bounds(image).size.w;
    int16_t         height = gbitmap_get_bounds(image).size.h;
    uint16_t bytes_per_row = gbitmap_get_bytes_per_row(image);
    #ifdef PBL_COLOR
    GBitmapFormat   format = gbitmap_get_format(image);
    uint8_t       *palette = (uint8_t*)gbitmap_get_palette(image);
    #endif
    
    // Bounds Checking -- feel free to remove this section if you know you won't go out of bounds
    int16_t           top = (start_y < 0) ? 0 - start_y : 0;
    int16_t          left = (start_x < 0) ? 0 - start_x : 0;
    int16_t        bottom = (height + start_y) > 168 ? 168 - start_y : height;
    int16_t         right = (width  + start_x) > 144 ? 144 - start_x : width;
    // End Bounds Checking

    
    for(int16_t y=top; y<bottom; ++y) {
      for(int16_t x=left; x<right; ++x) {
        #ifdef PBL_COLOR
        uint16_t addr = (y + start_y) * 144 + x + start_x;  // memory address of the pixel on the screen currently being colored
        switch(format) {
          case GBitmapFormat1Bit:        screen[addr] =                                     ((data[y*bytes_per_row + (x>>3)] >> ((7-(x&7))   )) &  1) ? GColorWhiteARGB8 : GColorBlackARGB8; break;
          case GBitmapFormat8Bit:        screen[addr] = combine_colors(screen[addr],          data[y*bytes_per_row +  x    ]);                          break;
          case GBitmapFormat1BitPalette: screen[addr] = combine_colors(screen[addr], palette[(data[y*bytes_per_row + (x>>3)] >> ((7-(x&7))   )) &  1]); break;
          case GBitmapFormat2BitPalette: screen[addr] = combine_colors(screen[addr], palette[(data[y*bytes_per_row + (x>>2)] >> ((3-(x&3))<<1)) &  3]); break;
          case GBitmapFormat4BitPalette: screen[addr] = combine_colors(screen[addr], palette[(data[y*bytes_per_row + (x>>1)] >> ((1-(x&1))<<2)) & 15]); break;
          default: break;
        }
        #endif
        
        #ifdef PBL_BW
          uint16_t addr = ((y + start_y) * 20) + ((x + start_x) >> 3);             // the screen memory address of the 8bit byte where the pixel is
          uint8_t  xbit = (x + start_x) & 7;                                       // which bit is the pixel inside of 8bit byte
          screen[addr] &= ~(1<<xbit);                                              // assume pixel to be black
          screen[addr] |= ((data[(y*bytes_per_row) + (x>>3)] >> (x&7))&1) << xbit; // draw white pixel if image has a white pixel
        #endif
      }
    }
    graphics_release_frame_buffer(ctx, framebuffer);
  }
}

// This is to better understand how the function above works
void draw_image_explaination(GContext *ctx, GBitmap *image) {      // Doesn't have start_x or start_y, so always puts image in the upper-right corner (for simplicity)
  GBitmap *framebuffer = graphics_capture_frame_buffer(ctx);       // Get framebuffer
  if(framebuffer) {                                                // If successfully captured the framebuffer
    uint8_t        *screen = gbitmap_get_data(framebuffer);        // Get pointer to framebuffer data

    uint8_t          *data = (uint8_t*)gbitmap_get_data(image);    // Get pointer to image data
    //GRect         bounds = gbitmap_get_bounds(image);            // The bounds of the image, if you need it
    int16_t          width = gbitmap_get_bounds(image).size.w;     // The width of the image in pixels
    int16_t         height = gbitmap_get_bounds(image).size.h;     // The height of the image in pixels
    uint16_t bytes_per_row = gbitmap_get_bytes_per_row(image);     // The number of bytes needed to represent each row in the image
    #ifdef PBL_COLOR
    GBitmapFormat   format = gbitmap_get_format(image);            // The format of the image.  On B&W, format should always be GBitmapFormat1Bit
    uint8_t       *palette = (uint8_t*)gbitmap_get_palette(image); // An array of colors, if the format is one of the Palette formats
    #endif

    // Bounds Checking -- Since x&y=0 in this function, don't need to bounds check top or left, just the size of the image
    if(width  > 144)  width = 144;
    if(height > 168) height = 168;
    // End Bounds Checking
    
    //uint8_t color;
    for(int16_t y=0; y<height; ++y) {
      for(int16_t x=0; x<width; ++x) {
        #ifdef PBL_COLOR
        uint16_t pixels_per_byte;                                       // The number of pixels that are represented in an 8-bit byte
        uint16_t screen_x_address =  x;                                 // if not starting in upper-right corner, would be =  x + start_x;
        uint16_t screen_y_address =  y * 144;                           // if not starting in upper-right corner, would be = (y + start_y) * 144;
        uint16_t screen_address = screen_y_address + screen_x_address;  // The memory address for the pixel on the screen we're writing to
        
        switch(format) {                                                // This switch could be an array
          case GBitmapFormat1Bit:        pixels_per_byte = 8; break;
          case GBitmapFormat8Bit:        pixels_per_byte = 1; break;
          case GBitmapFormat1BitPalette: pixels_per_byte = 8; break;
          case GBitmapFormat2BitPalette: pixels_per_byte = 4; break;
          case GBitmapFormat4BitPalette: pixels_per_byte = 2; break;
          default:                       pixels_per_byte = 0; break;  // This might break it, actually... Need to test with a bad image.
        }
        uint16_t bits_per_pixel   = 8 / pixels_per_byte;      // How many bits are needed to represent 1 pixel.
        uint16_t number_of_colors = power(2, bits_per_pixel); // Number of colors a pixel can be = 2^bits_per_pixel (e.g. 3 bits get you 2^3 = 2*2*2 = 8 colors)
        uint16_t image_y_address  = y * bytes_per_row;        // The row in the image containing the pixel to draw
        uint16_t image_x_address  = x / pixels_per_byte;      // The byte offset from the left edge of the picture at row image_y_address containing the pixel to draw
        uint16_t image_address = image_y_address + image_x_address; // The byte offset from the beginning of the image data containing the pixel to draw
        uint16_t color = (data[image_address] >> (((pixels_per_byte - 1) - (x % pixels_per_byte)) * bits_per_pixel)) % number_of_colors; // color = the color or pallette position to draw
        // the "(pixels_per_byte - 1) - " part is due to Basalt storing pixels in the backwards endianess compared to Aplite
        // All the divides and % modulus is just to make things easier to understand.  Normally it'd use >> bitshifts and & masks instead (respectively). (see draw_image())
        switch(format) {
          case GBitmapFormat1Bit:        color = color ? GColorWhiteARGB8 : GColorBlackARGB8; break; // Before: color = 0 or 1.  Now: color = Black or White
          case GBitmapFormat8Bit:        color = color;                                       break; // Nothing to do, color is already the right color.
          case GBitmapFormat1BitPalette: color = palette[color];                              break; // Before: color = palette position. Now: color = palette color
          case GBitmapFormat2BitPalette: color = palette[color];                              break; // Before: color = palette position. Now: color = palette color
          case GBitmapFormat4BitPalette: color = palette[color];                              break; // Before: color = palette position. Now: color = palette color
          default: break;
        }
        color = combine_colors(screen[screen_address], color);  // This enables transparency.  Comment this line out to disable transparency
        screen[screen_address] = color;  // Set the pixel on the screen
        #endif
        #ifdef PBL_BW
        uint16_t screen_y_addr = y * 20;  // memory offset of the leftmost pixel in row Y (screen has 20 bytes per row) = 0*20 to 167*20
        uint16_t screen_x_addr = x / 8;  // how many bytes to the right in row Y to the byte where the pixel is (screen has 8 pixels per byte) = 0 to 18
        uint16_t   screen_addr = screen_x_addr + screen_y_addr; // memory address of the byte where the screen pixel is in
        uint8_t  screen_x_bit  = x % 8;   // which bit inside of 8bit byte the pixel is in = 0 to 7
        
        uint16_t image_y_addr  = y * bytes_per_row;  // memory offset of the leftmost pixel in row Y (image has "bytes_per_row" bytes per row)
        uint16_t image_x_addr  = x / 8;  // image has 8 pixels per byte, so x's memory address will be the same for each set of 8 pixels
        uint16_t image_addr    = image_x_addr + image_y_addr; // offset of the 8bit byte of where in the image the pixel is in
        uint8_t  image_x_bit   = x % 8;   // which bit inside of the 8bit byte the pixel is in = 0 to 7
        
        uint8_t  image_bit = (data[image_addr] >> image_x_bit) & 1; // bit will = 0 or 1 depending on if image pixel is black(0) or white(1)
        
        //screen[screen_addr] |=          (1 << screen_x_bit); // set pixel to white
          screen[screen_addr] &=         ~(1 << screen_x_bit); // set pixel to black
          screen[screen_addr] |=  (image_bit << screen_x_bit); // draw white pixel if image bit is 1, else keep pixel the same color
        //screen[screen_addr] &= ~(image_bit << screen_x_bit); // draw black pixel if image bit is 1, else keep pixel the same color
        #endif
      }
    }
    graphics_release_frame_buffer(ctx, framebuffer);
  }
}



// Draw the textbox with the current picture information
void draw_image_info(GContext *ctx, GBitmap *bmp) {
    int16_t          width = gbitmap_get_bounds(bmp).size.w;
    int16_t         height = gbitmap_get_bounds(bmp).size.h;
    uint16_t bytes_per_row = gbitmap_get_bytes_per_row(bmp);
    GBitmapFormat   format = gbitmap_get_format(bmp);

    GRect textbox = GRect(3, 90, 138, 74);
    fill_rect(ctx, textbox, 0b01000000);    // If on color, draw translucent box
    
    char text[100];  //Buffer to hold text
    snprintf(text, sizeof(text), "Image Number: %d / %d\nSize: %d x %d \n%s\n%d Bytes/row\nNumber of Colors: %d", current_image+1, number_of_images, width, height, get_gbitmapformat_text(format), bytes_per_row, (int)get_number_ofcolors(format));
    textbox.origin.x += 4; textbox.origin.y += 1;
    graphics_context_set_text_color(ctx, GColorBlack); graphics_draw_text(ctx, text, fonts_get_system_font("RESOURCE_ID_GOTHIC_14"), textbox, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    textbox.origin.x -= 2; textbox.origin.y -= 2;
    graphics_context_set_text_color(ctx, GColorWhite); graphics_draw_text(ctx, text, fonts_get_system_font("RESOURCE_ID_GOTHIC_14"), textbox, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

// ------------------------------------------------------------------------ //
//  Button Functions
// ------------------------------------------------------------------------ //
void     up_single_click_handler(ClickRecognizerRef recognizer, void *context) {--current_image; if(current_image>=number_of_images) current_image = number_of_images-1; layer_mark_dirty(graphics_layer);}
void select_single_click_handler(ClickRecognizerRef recognizer, void *context) { }
void   down_single_click_handler(ClickRecognizerRef recognizer, void *context) {++current_image; if(current_image>=number_of_images) current_image = 0;                  layer_mark_dirty(graphics_layer);}
  
void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
}

// ------------------------------------------------------------------------ //
//  Layer Drawing Functions
// ------------------------------------------------------------------------ //
void graphics_layer_update(Layer *me, GContext *ctx) {
  //graphics_draw_bitmap_in_rect(ctx, image[current_image], gbitmap_get_bounds(image[current_image]));  // How you'd normally draw an image to the screen at screen location: x=0, y=0
  draw_image(ctx, image[current_image], 0, 0);  // Draw image to screen buffer at screen location: x=0, y=0
  //draw_image_explaination(ctx, image[current_image]);
  draw_image_info(ctx, image[current_image]);   // Draw the textbox with the current picture information
}

  
// ------------------------------------------------------------------------ //
//  Main Functions
// ------------------------------------------------------------------------ //
static void main_window_load(Window *window) {
  root_layer = window_get_root_layer(window);

  graphics_layer = layer_create(layer_get_frame(root_layer));
  layer_add_child(root_layer, graphics_layer);
  layer_set_update_proc(graphics_layer, graphics_layer_update);
}

static void main_window_unload(Window *window) {
  layer_destroy(graphics_layer);
}
  
static void init(void) {
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {
      .load = main_window_load,
    .unload = main_window_unload
  });
  window_set_click_config_provider(main_window, click_config_provider);
  window_set_background_color(main_window, IF_COLORBW(GColorLightGray, GColorWhite));  // Background is White on B&W and Grey on Color
  IF_BW(window_set_fullscreen(main_window, true));  // Fullscreen if on OG Pebble
  
  load_graphics();                      // Load Images into Memory
  window_stack_push(main_window, true); // Display window
}
  
static void deinit(void) {
  unload_graphics();
  window_destroy(main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}