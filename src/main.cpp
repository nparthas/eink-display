#include <ft2build.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <bitset>
#include <ctime>
#include <cwctype>
#include <iostream>
#include <sstream>

#include "project/epaper.h"

using namespace epaper;

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
    auto paper = Epaper();

    paper.SetUpIos();

    paper.InitFullUpdate();
    paper.ClearDisplay();
    paper.Wait(200);

    auto image = Bitmap(Epaper::kHeight, Epaper::kWidth);

    auto weather_renderer = TextRenderer(65, TextRenderer::Fonts::kWeather);
    auto text_renderer    = TextRenderer(44, TextRenderer::Fonts::kLetterBoard);

    auto weather = weather_renderer.RenderText(L"\uf00d");
    weather_renderer.DrawOnImage(image, weather, 0, 0);

    auto time = text_renderer.RenderText(get_timestring());
    weather_renderer.DrawOnImage(image, time, 0, Epaper::kWidth - time.width());

    auto description = text_renderer.RenderText(L"-28° CLOUDY");
    weather_renderer.DrawOnImage(image, description, weather.height(), 0);

    auto smaller_weather_renderer = TextRenderer(37, TextRenderer::Fonts::kWeather);
    auto text_lower               = text_renderer.RenderText(L"-25");
    auto text_lower_icon          = smaller_weather_renderer.RenderText(L"\uf018");

    auto offset = weather.height() + 4;
    weather_renderer.DrawOnImage(image, text_lower_icon, offset, description.width() + 12);

    offset += text_lower_icon.height() + 4;
    weather_renderer.DrawOnImage(image, text_lower, offset, description.width() + 12);

    offset += text_lower.height() + 4;
    weather_renderer.DrawOnImage(image, text_lower_icon, offset, description.width() + 12);

    offset += text_lower_icon.height() + 4;
    weather_renderer.DrawOnImage(image, text_lower, offset, description.width() + 12);

    image.Print();

    paper.DisplayImage(image.Raw());

    std::wcout << "Press <Enter> to continue..." << std::endl;
    std::cin.get();

    paper.ClearDisplay();
    paper.Shutdown();

    return 0;
}