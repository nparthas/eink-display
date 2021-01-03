#include "project/epaper.h"

namespace epaper {

void Epaper::set_gpio_mode_(int pin, GpioMode mode) {
    // macros come from wiringpi library
    if (mode == GpioModeInput) {
        pinMode(pin, INPUT);
        pullUpDnControl(pin, PUD_UP);
    } else {
        pinMode(pin, OUTPUT);
    }
}

void Epaper::send_command_(uint8_t reg) {
    digitalWrite(this->kPinDc, 0);
    digitalWrite(this->kPinCs, 0);
    wiringPiSPIDataRW(this->kSpiChannel, &reg, 1);
    digitalWrite(this->kPinCs, 1);
}

void Epaper::send_data_(uint8_t data) {
    digitalWrite(this->kPinDc, 1);
    digitalWrite(this->kPinCs, 0);
    wiringPiSPIDataRW(this->kSpiChannel, &data, 1);
    digitalWrite(this->kPinCs, 1);
}

void Epaper::turn_display_on_(void) {
    this->send_command_(0x22);
    this->send_data_(0xC7);
    this->send_command_(0x20);
    this->BusyWait();
}

void Epaper::SetUpIos() {
    // init device connection
    if (wiringPiSetupGpio() < 0) {
        std::wcout << "failed to set up wiringpi library" << std::endl;
    }

    this->set_gpio_mode_(this->kPinRst, GpioModeOutput);
    this->set_gpio_mode_(this->kPinDc, GpioModeOutput);
    this->set_gpio_mode_(this->kPinCs, GpioModeOutput);
    this->set_gpio_mode_(this->kPinBusy, GpioModeInput);

    digitalWrite(this->kPinCs, 1);

    wiringPiSPISetup(0, 10000000);
    this->Wait(200);
}

void Epaper::Shutdown() {
    this->DeepSleep();

    // deinit device connection
    this->Wait(2000);
    digitalWrite(this->kPinCs, GpioModeInput);
    digitalWrite(this->kPinDc, GpioModeInput);
    digitalWrite(this->kPinRst, GpioModeInput);
}

void Epaper::InitFullUpdate(void) {
    this->Reset();

    this->BusyWait();
    this->send_command_(0x12);  // soft reset
    this->BusyWait();

    this->send_command_(0x74);  // set analog block control
    this->send_data_(0x54);
    this->send_command_(0x7E);  // set digital block control
    this->send_data_(0x3B);

    this->send_command_(0x01);  // Driver output control
    this->send_data_(0xF9);
    this->send_data_(0x00);
    this->send_data_(0x00);

    this->send_command_(0x11);  // data entry mode
    this->send_data_(0x01);

    this->send_command_(0x44);  // set Ram-X address start/end position
    this->send_data_(0x00);
    this->send_data_(0x0F);  // 0x0C-->(15+1)*8=128

    this->send_command_(0x45);  // set Ram-Y address start/end position
    this->send_data_(0xF9);     // 0xF9-->(249+1)=250
    this->send_data_(0x00);
    this->send_data_(0x00);
    this->send_data_(0x00);

    this->send_command_(0x3C);  // BorderWavefrom
    this->send_data_(0x03);

    this->send_command_(0x2C);  // VCOM Voltage
    this->send_data_(0x55);

    this->send_command_(0x03);
    this->send_data_(this->kLutFullUpdate[70]);

    this->send_command_(0x04);  //
    this->send_data_(this->kLutFullUpdate[71]);
    this->send_data_(this->kLutFullUpdate[72]);
    this->send_data_(this->kLutFullUpdate[73]);

    this->send_command_(0x3A);  // Dummy Line
    this->send_data_(this->kLutFullUpdate[74]);
    this->send_command_(0x3B);  // Gate time
    this->send_data_(this->kLutFullUpdate[75]);

    this->send_command_(0x32);
    for (uint16_t i = 0; i < 70; i++) {
        this->send_data_(this->kLutFullUpdate[i]);
    }

    this->send_command_(0x4E);  // set RAM x address count to 0;
    this->send_data_(0x00);
    this->send_command_(0x4F);  // set RAM y address count to 0X127;
    this->send_data_(0xF9);
    this->send_data_(0x00);
    this->BusyWait();
}

void Epaper::Reset(void) {
    digitalWrite(this->kPinRst, 1);
    this->Wait(200);
    digitalWrite(this->kPinRst, 0);
    this->Wait(2);
    digitalWrite(this->kPinRst, 1);
    this->Wait(200);
}

void Epaper::BusyWait(void) {
    while (digitalRead(this->kPinBusy) == HIGH) {  // LOW: idle, HIGH: busy
        this->Wait(50);
    }
}

void Epaper::ClearDisplay(void) {
    this->send_command_(0x24);
    for (uint8_t j = 0; j < this->kHeightBound; j++) {
        for (uint8_t i = 0; i < this->kWidthBound; i++) {
            this->send_data_(0XFF);
        }
    }

    this->turn_display_on_();
}

void Epaper::DisplayImage(uint8_t* image) {
    this->send_command_(0x24);
    for (uint16_t j = 0; j < this->kHeightBound; j++) {
        for (uint16_t i = 0; i < this->kWidthBound; i++) {
            this->send_data_(image[i + j * this->kWidthBound]);
        }
    }
    this->turn_display_on_();
}

void Epaper::DeepSleep(void) {
    this->send_command_(0x22);  // POWER OFF
    this->send_data_(0xC3);
    this->send_command_(0x20);

    this->send_command_(0x10);  // enter deep sleep
    this->send_data_(0x01);
    this->Wait(100);
}

void Epaper::Wait(int64_t msec) { std::this_thread::sleep_for(std::chrono::milliseconds(msec)); }

Bitmap::Bitmap(uint16_t height, uint16_t width)
    : height_(height),
      width_(width),
      height_bound_(height),
      width_bound_((width % 8 == 0) ? (width / 8) : (width / 8 + 1)) {
    this->buffer_raw_ = std::make_unique<uint8_t[]>(this->width_bound_ * this->height_bound_);
    this->buffer_     = std::make_unique<uint8_t*[]>(this->height_bound_);

    for (uint16_t i = 0; i < this->height_bound_; i++) {
        this->buffer_[i] = &this->buffer_raw_[i * this->width_bound_];
    }

    this->ClearWhite();
}

Bitmap::Bitmap(Bitmap&& move) noexcept
    : height_(move.height_),
      width_(move.width_),
      height_bound_(move.height_bound_),
      width_bound_(move.width_bound_) {
    move.height_       = 0;
    move.width_        = 0;
    move.height_bound_ = 0;
    move.width_bound_  = 0;

    this->buffer_raw_ = std::move(move.buffer_raw_);
    this->buffer_     = std::move(move.buffer_);
}

Bitmap& Bitmap::operator=(Bitmap&& move) noexcept {
    if (this == &move) {
        return *this;
    }

    this->height_       = std::exchange(move.height_, 0);
    this->width_        = std::exchange(move.width_, 0);
    this->height_bound_ = std::exchange(move.height_bound_, 0);
    this->width_bound_  = std::exchange(move.width_bound_, 0);

    this->buffer_raw_ = std::move(move.buffer_raw_);
    this->buffer_     = std::move(move.buffer_);

    return *this;
}

void Bitmap::ClearWhite() {
    std::fill_n(this->buffer_raw_.get(), this->width_bound_ * this->height_bound_,
                this->kWhiteBlock);
}

void Bitmap::ClearBlack() {
    std::fill_n(this->buffer_raw_.get(), this->width_bound_ * this->height_bound_,
                this->kBlackBlock);
}

void Bitmap::Invert() {
    for (int i = 0; i < this->height_bound_ * this->width_bound_; i++) {
        this->buffer_raw_[i] = ~this->buffer_raw_[i];
    }
}

uint8_t* Bitmap::Raw() { return this->buffer_raw_.get(); }

void Bitmap::Print() const {
    for (uint16_t i = 0; i < this->height_bound_; i++) {
        for (uint16_t j = 0; j < this->width_bound_; j++) {
            std::wcout << std::bitset<8>(this->buffer_[i][j]);
        }

        std::wcout << std::endl;
    }
}

uint8_t& Bitmap::operator[](size_t idx) { return this->buffer_raw_[idx]; }

const uint8_t& Bitmap::operator[](size_t idx) const { return this->buffer_raw_[idx]; }

uint8_t& Bitmap::operator()(size_t height, size_t width) { return this->buffer_[height][width]; }

const uint8_t& Bitmap::operator()(size_t height, size_t width) const {
    return this->buffer_[height][width];
}

void Renderer::DrawOnImage(Bitmap& target, Bitmap& src, uint16_t start_height, uint16_t start_width,
                           RenderAction action) {
    for (uint16_t i = 0; i < src.height_bound(); i++) {
        for (uint16_t j = 0; j < src.width_bound(); j++) {
            if (i + start_height >= target.height_bound() ||
                j + start_width / 8 >= target.width_bound()) {
                continue;
            }

            switch (action) {
                case kRenderActionReplace:
                    target(i + start_height, j + start_width / 8) = src(i, j);
                    break;
                case kRenderActionAnd:
                    target(i + start_height, j + start_width / 8) &= src(i, j);
                    break;
                case kRenderActionOr:
                    target(i + start_height, j + start_width / 8) |= src(i, j);
                    break;
            }
        }
    }
}

void TextRenderer::print_FT_Bitmap(FT_Bitmap* bitmap) const {
    for (uint16_t i = 0; i < bitmap->rows; i++) {
        for (uint16_t j = 0; j < bitmap->width; j++)
            std::wcout << (bitmap->buffer[i * bitmap->width + j] == 0
                               ? " "
                               : bitmap->buffer[i * bitmap->width + j] < 128 ? "+" : "*");
        std::wcout << std::endl;
    }
    std::wcout << std::endl;
    ;
}

TextRenderer::TextRenderer(uint16_t size, std::string font) : size_(size), font_(font) {
    FT_Error error = FT_Init_FreeType(&this->library_);
    if (error) {
        std::wcout << "error" << std::endl;
    }

    error = FT_New_Face(this->library_, font.c_str(), 0, &this->face_);
    if (error) {
        std::wcout << "error1" << std::endl;
    }

    error = FT_Set_Pixel_Sizes(this->face_, 0, this->size_);
    if (error) {
        std::wcout << "error2" << std::endl;
    }

    this->slot_ = this->face_->glyph;
}

TextRenderer::~TextRenderer() {
    this->slot_ = nullptr;
    FT_Done_Face(this->face_);
    FT_Done_FreeType(this->library_);
}

Bitmap TextRenderer::RenderText(std::wstring const text) {
    // render text with a 90 degree rotation

    FT_Bitmap* bitmap         = &this->slot_->bitmap;
    uint16_t   target_width   = 0;
    uint16_t   target_height  = 0;
    uint16_t   target_advance = 0;
    uint16_t   pad_size       = 0;
    // loop through the characters to find the required sizes and gaps
    for (auto const& character : text) {
        FT_Error error = FT_Load_Char(this->face_, character, FT_LOAD_NO_BITMAP);
        if (error) {
            std::wcout << "error3" << std::endl;
        }

        if (target_width < bitmap->rows) {
            target_width = bitmap->rows;
        }

        if (target_height < bitmap->width) {
            target_height = bitmap->width;
        }

        // advance.x is in 26.6 fixed point notation - transform to pixel count
        if (target_advance < this->slot_->advance.x / 64) {
            target_advance = this->slot_->advance.x / 64;
        }

        if ((target_advance > target_height) && target_advance - target_height > pad_size) {
            pad_size = target_advance - target_height;
        }
    }

    Bitmap image = Bitmap(std::max(target_height, target_advance) * text.size(), target_width);
    image.ClearBlack();

    uint16_t k = 0;
    for (auto const& character : text) {
        // map the 8bit pixels to single bit pixels
        FT_Error error = FT_Load_Char(this->face_, character, FT_LOAD_RENDER);
        if (error) {
            std::wcout << "error3" << std::endl;
        }

        // space character does not get rendered
        // just increment k by the characters it would take up
        if (character == L' ') {
            k += ((image.width_bound() * 8) * (std::min(target_advance, target_height) / 2));
            continue;
        }

        // align all of the caracters on their base, requires shifing shorter characters down (on
        // correctly oriented image) add additional padding to the start of each row and take it
        // away from the padding at the end
        // special case some characters for distinct alignment, otherwise align bottom
        uint16_t start_padding = (target_width - bitmap->rows);

        if (character == L':' || character == L'-') {
            start_padding /= 2;
        } else if (character == L'°') {
            start_padding = 0;
        }

        for (uint16_t j = 0; j < bitmap->width; j++) {
            k += start_padding;
            for (uint16_t i = 0; i < bitmap->rows; i++) {
                uint8_t val = (bitmap->buffer[bitmap->width * i + j] == 0) ? 0 : 1 << (7 - (k % 8));
                image[k++ / 8] |= val;
            }

            // add the width bound padding for byte to bit packing to the end
            k += image.width_bound() * 8 - bitmap->rows - start_padding;
        }

        // pad the characters with the suggested width
        k += (image.width_bound() * 8) *
             std::max(((this->slot_->advance.x / 64) - bitmap->width), 1ul);
    }

    image.Invert();

    return image;
}

}  // namespace epaper
