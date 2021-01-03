#pragma once

#include <ft2build.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include FT_FREETYPE_H

#include <math.h>

#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>

namespace epaper {

class Epaper {
   public:
    const uint8_t kSpiChannel = 0;

    static constexpr uint8_t kHeight = 250;
    static constexpr uint8_t kWidth  = 122;

    void SetUpIos(void);                // set up pin ios and spi bus
    void Shutdown(void);                // shutdown routine including reseting all configured ios
    void InitFullUpdate(void);          // init device for full update
    void Reset(void);                   // hard reset of device
    void BusyWait(void);                // busywait on busy pin
    void ClearDisplay(void);            // clear screen (all pixels white)
    void DisplayImage(uint8_t* image);  // display image
    void DeepSleep(void);               // enter deep sleep / low power mode
    void Wait(int64_t msec);            // wait for msec

   private:
    enum GpioMode { GpioModeInput = 0, GpioModeOutput = 1 };
    const uint8_t kPinRst  = 17;
    const uint8_t kPinDc   = 25;
    const uint8_t kPinCs   = 8;
    const uint8_t kPinBusy = 24;

    const uint16_t kHeightBound = this->kHeight;
    const uint16_t kWidthBound =
        (this->kWidth % 8 == 0) ? (this->kWidth / 8) : (this->kWidth / 8 + 1);

    const uint8_t kLutFullUpdate[76] = {
        0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,  // LUT0: BB:     VS 0 ~7
        0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00,  // LUT1: BW:     VS 0 ~7
        0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00,  // LUT2: WB:     VS 0 ~7
        0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00,  // LUT3: WW:     VS 0 ~7
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LUT4: VCOM:   VS 0 ~7

        0x03, 0x03, 0x00, 0x00, 0x02,  // TP0 A~D RP0
        0x09, 0x09, 0x00, 0x00, 0x02,  // TP1 A~D RP1
        0x03, 0x03, 0x00, 0x00, 0x02,  // TP2 A~D RP2
        0x00, 0x00, 0x00, 0x00, 0x00,  // TP3 A~D RP3
        0x00, 0x00, 0x00, 0x00, 0x00,  // TP4 A~D RP4
        0x00, 0x00, 0x00, 0x00, 0x00,  // TP5 A~D RP5
        0x00, 0x00, 0x00, 0x00, 0x00,  // TP6 A~D RP6

        0x15, 0x41, 0xA8, 0x32, 0x30, 0x0A,
    };

    // const uint8_t kLutPartialUpdate[76] = {
    //     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LUT0: BB:     VS 0 ~7
    //     0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LUT1: BW:     VS 0 ~7
    //     0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LUT2: WB:     VS 0 ~7
    //     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LUT3: WW:     VS 0 ~7
    //     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LUT4: VCOM:   VS 0 ~7

    //     0x0A, 0x00, 0x00, 0x00, 0x00,  // TP0 A~D RP0
    //     0x00, 0x00, 0x00, 0x00, 0x00,  // TP1 A~D RP1
    //     0x00, 0x00, 0x00, 0x00, 0x00,  // TP2 A~D RP2
    //     0x00, 0x00, 0x00, 0x00, 0x00,  // TP3 A~D RP3
    //     0x00, 0x00, 0x00, 0x00, 0x00,  // TP4 A~D RP4
    //     0x00, 0x00, 0x00, 0x00, 0x00,  // TP5 A~D RP5
    //     0x00, 0x00, 0x00, 0x00, 0x00,  // TP6 A~D RP6

    //     0x15, 0x41, 0xA8, 0x32, 0x30, 0x0A,
    // };

    void set_gpio_mode_(int pin, GpioMode mode);  // set pin gpio mode
    void send_command_(uint8_t reg);              // send command to command register
    void send_data_(uint8_t data);                // send data
    void turn_display_on_(void);                  // turn on display
};

// for monochrome images
class Bitmap {
   public:
    uint16_t height() const { return this->height_; }              // get height in pixels
    uint16_t width() const { return this->width_; }                // get width in pixels
    uint16_t height_bound() const { return this->height_bound_; }  // get height in padded bytes
    uint16_t width_bound() const { return this->width_bound_; }    // get width in bytes

    // height and width in pixel values - initialized to white only image
    Bitmap(uint16_t height, uint16_t width);
    Bitmap(Bitmap&& move) noexcept;

    Bitmap& operator=(Bitmap&& move) noexcept;

    void ClearWhite();  // clears the image to white
    void ClearBlack();  // clears the image to black
    void Invert();      // inverts the pixel values 1 -> 0 / 0 -> 1

    uint8_t* Raw();  // returns raw buffer

    void Print() const;  // print image to terminal

    // index bitmap using 1d logic
    uint8_t&       operator[](size_t idx);
    const uint8_t& operator[](size_t idx) const;

    // index bitmap using 2d logic
    uint8_t&       operator()(size_t height, size_t width);
    const uint8_t& operator()(size_t height, size_t width) const;

   private:
    const uint8_t kWhiteBlock = 0xFF;
    const uint8_t kBlackBlock = 0x00;

    // pixel dimension values height = y, width = x
    uint16_t height_;
    uint16_t width_;

    // byte bounds for storing in 8 bit fields
    uint16_t height_bound_;
    uint16_t width_bound_;

    // buffer of actual image data
    std::unique_ptr<uint8_t*[]> buffer_;      // 2d array [row][col]
    std::unique_ptr<uint8_t[]>  buffer_raw_;  // 1d array with all of the continguous
};

class Renderer {
   public:
    enum RenderAction {
        kRenderActionReplace = 0,
        kRenderActionAnd,
        kRenderActionOr,
    };

    virtual ~Renderer() {}

    // draw on image using the selection action from the given starting location
    void DrawOnImage(Bitmap& target, Bitmap& src, uint16_t start_height, uint16_t start_width,
                     RenderAction action = kRenderActionAnd);
};

class TextRenderer : public Renderer {
   public:
    struct Fonts {
        static constexpr const char* kWeather     = "resources/weathericons-regular-webfont.ttf";
        static constexpr const char* kRobinson    = "resources/Robinson Regular.otf";
        static constexpr const char* kVt323       = "resources/VT323-Regular.ttf";
        static constexpr const char* kDroidSans   = "resources/DroidSansMono.ttf";
        static constexpr const char* kLetterBoard = "resources/ShockingHeadline-vYKA.ttf";
        // static constexpr const char* kLetterBoard = "resources/LetterboardLite-Semibold.ttf";
    };

    TextRenderer(uint16_t size, std::string font);
    virtual ~TextRenderer();

    Bitmap RenderText(std::wstring text);

   private:
    uint16_t    size_;
    std::string font_;

    FT_Library   library_;
    FT_Face      face_;
    FT_GlyphSlot slot_;

    void print_FT_Bitmap(FT_Bitmap* bitmap) const;
};

}  // namespace epaper
