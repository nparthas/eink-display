#include <ft2build.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include FT_FREETYPE_H

#include <bitset>
#include <iostream>

#include "project/epaper.h"

using namespace epaper;

int main(void) {
    auto paper = Epaper();

    paper.SetUpIos();

    paper.InitFullUpdate();
    paper.ClearDisplay();
    paper.Wait(200);

    auto image = Bitmap(Epaper::kHeight, Epaper::kWidth);

    auto weather_renderer = TextRenderer(70, TextRenderer::Fonts::kWeather);

    auto weather = weather_renderer.RenderText(L"\uf00d");
    weather_renderer.DrawOnImage(image, weather, 0, 0);

    auto text_renderer = TextRenderer(40, TextRenderer::Fonts::kRobinson);

    auto text = text_renderer.RenderText(L"28°C");
    std::cout << +text.height() << " " << +text.width() <<std::endl;
    weather_renderer.DrawOnImage(image, text, weather.height(), 0);

    auto description = text_renderer.RenderText(L"Clear");
    weather_renderer.DrawOnImage(image, description, weather.height(), text.width() + 4);

    auto time = text_renderer.RenderText(L"13:20");
    weather_renderer.DrawOnImage(image, time, weather.height(),
                                 text.width() + description.width() + 6);

    image.Print();

    paper.DisplayImage(image.Raw());

    std::wcout << "Press <Enter> to continue..." << std::endl;
    std::cin.get();

    paper.ClearDisplay();
    paper.Shutdown();

    return 0;
}