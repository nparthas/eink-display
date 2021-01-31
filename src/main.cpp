#include <ft2build.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <array>
#include <bitset>
#include <ctime>
#include <cwctype>
#include <iostream>

#include "project/epaper.h"

using namespace epaper;

constexpr uint16_t kWeatherFontSize = 65;
constexpr uint16_t kTextFontSize    = 42;
constexpr uint16_t kSubTextFontSize = 37;
constexpr uint16_t kStaticHeightOffset = 4;
constexpr uint16_t kStaticWidthOffset = 12;
constexpr uint16_t kZeroHeight = 0;
constexpr uint16_t kZeroWidth = 0;

std::wstring get_timestring() {
    const static std::wstring DAY[]   = {L"Sunday",   L"Monday", L"Tuesday", L"Wednesday",
                                       L"Thursday", L"Friday", L"Saturday"};
    const static std::wstring MONTH[] = {L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
                                         L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};

    std::time_t        time_temp = std::time(nullptr);
    std::tm*           time_out  = std::localtime(&time_temp);
    std::wstringstream wss;

    wss << DAY[time_out->tm_wday] << " " << MONTH[time_out->tm_mon] << " " << time_out->tm_mday
        << ((time_out->tm_hour < 10) ? " 0" : " ") << time_out->tm_hour
        << ((time_out->tm_min < 10) ? ":0" : ":") << time_out->tm_min;

    std::wstring datetime = wss.str();

    std::transform(datetime.begin(), datetime.end(), datetime.begin(), std::towupper);

    return datetime;
}

int main(void) {
    Weather w("api-key-template.json");
    auto    forecast = w.GetForecast();

    if (!forecast) {
        std::wcout << "was unable to get forecast" << std::endl;
        return -1;
    }

    auto paper = Epaper();

    paper.SetUpIos();

    paper.InitFullUpdate();
    paper.ClearDisplay();
    paper.Wait(200);

    auto image = Bitmap(Epaper::kHeight, Epaper::kWidth);

    auto weather_renderer = TextRenderer(kWeatherFontSize, TextRenderer::Fonts::kWeather);
    auto text_renderer    = TextRenderer(kTextFontSize, TextRenderer::Fonts::kLetterBoard);

    auto time = text_renderer.RenderText(get_timestring());
    weather_renderer.DrawOnImage(image, time, kZeroHeight, Epaper::kWidth - time.width());

    auto weather = weather_renderer.RenderText(forecast->icons[0]);
    weather_renderer.DrawOnImage(image, weather, kZeroHeight,
                                 ((Epaper::kWidth - time.width()) - weather.width()) / 2);

    auto description = text_renderer.RenderText(std::to_wstring(forecast->temperatures[0]) + L"° " +
                                                forecast->description);
    weather_renderer.DrawOnImage(image, description, weather.height() + kStaticHeightOffset - 2, kZeroWidth);

    auto smaller_weather_renderer = TextRenderer(kSubTextFontSize, TextRenderer::Fonts::kWeather);
    auto text_lower = text_renderer.RenderText(std::to_wstring(forecast->temperatures[1]) + L"°");
    auto text_lower_icon = smaller_weather_renderer.RenderText(forecast->icons[1]);

    auto offset = weather.height() + kStaticHeightOffset;
    weather_renderer.DrawOnImage(image, text_lower_icon, offset, description.width() + kStaticWidthOffset);

    offset += text_lower_icon.height() + kStaticHeightOffset;
    weather_renderer.DrawOnImage(image, text_lower, offset, description.width() + kStaticWidthOffset);

    text_lower      = text_renderer.RenderText(std::to_wstring(forecast->temperatures[2]) + L"°");
    text_lower_icon = smaller_weather_renderer.RenderText(forecast->icons[2]);

    offset += text_lower.height() + kStaticHeightOffset;
    weather_renderer.DrawOnImage(image, text_lower_icon, offset, description.width() + kStaticWidthOffset);

    offset += text_lower_icon.height() + kStaticHeightOffset;
    weather_renderer.DrawOnImage(image, text_lower, offset, description.width() + kStaticWidthOffset);

    image.Print();

    paper.DisplayImage(image.Raw());

    std::wcout << "Press <Enter> to continue..." << std::endl;
    std::wcin.get();

    paper.ClearDisplay();
    paper.Shutdown();

    return 0;
}
