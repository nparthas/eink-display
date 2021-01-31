#include <ft2build.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include FT_FREETYPE_H

#include <curl/curl.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <ctime>
#include <cwctype>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "project/epaper.h"
#include "third-party/json.hpp"

using namespace epaper;
using json = nlohmann::json;

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

class Weather {
   public:
    struct Forecast {
        std::array<int32_t, 4> temperatures;  // current temp and temp every 6 hours
        std::wstring           description;   // description string
        std::wstring           icon;          // icon unicode string
    };

    Weather(std::string keyfile) {
        (void)keyfile;

        this->url_ =
            "http://api.openweathermap.org/data/2.5/"
            "onecall?lat=43.4668&lon=-80.5164&exclude=alerts,minutely,daily&units=metric&lang=en&"
            "appid=4eb0d48c840649fa5c33be13c5f21e30";

        this->curl_ = curl_easy_init();
        this->data_ = std::make_unique<std::string>();

        curl_easy_setopt(this->curl_, CURLOPT_URL, this->url_.c_str());
        curl_easy_setopt(this->curl_, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        curl_easy_setopt(this->curl_, CURLOPT_TIMEOUT, 10);
        curl_easy_setopt(this->curl_, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_setopt(this->curl_, CURLOPT_WRITEFUNCTION, Weather::callback);
        curl_easy_setopt(this->curl_, CURLOPT_WRITEDATA, this->data_.get());
    }

    ~Weather() { curl_easy_cleanup(this->curl_); }

    std::unique_ptr<Forecast> GetForecast() {
        uint64_t return_code = 0;

        curl_easy_perform(this->curl_);
        curl_easy_getinfo(this->curl_, CURLINFO_RESPONSE_CODE, &return_code);

        if (return_code == 200) {
            auto j = json::parse(*this->data_.get(), nullptr, false);
            if (!j.is_discarded()) {
                std::unique_ptr<Forecast> forecast = std::make_unique<Forecast>();

                std::string description = j["current"]["weather"][0]["main"].get<std::string>();
                forecast->description   = std::wstring(description.begin(), description.end());

                int32_t image_id = j["current"]["weather"][0]["id"].get<int32_t>();
                forecast->icon   = this->get_icon_for_id(image_id);

                forecast->temperatures[0] = std::round(j["current"]["temp"].get<double>());
                for (int i = 1; i < 4; i++) {
                    forecast->temperatures[i] =
                        std::round(j["hourly"][i * 6 - 1]["temp"].get<double>());
                }

                return forecast;

            } else {
                std::wcout << "Got good response but with malformed json" << std::endl;
                return std::unique_ptr<Forecast>(nullptr);
            }
        } else {
            std::wcout << "Couldn't get weather, response code: " << return_code
                       << " retrying later" << std::endl;
            auto j = json::parse(*this->data_.get(), nullptr, false);
            if (!j.is_discarded()) {
                std::stringstream iss;
                iss << std::setw(4) << j;
                std::string  str  = iss.str();
                std::wstring wstr = std::wstring(str.begin(), str.end());
                std::wcout << wstr << std::endl;
            }

            return std::unique_ptr<Forecast>(nullptr);
        }
    }

   private:
    CURL*                        curl_;
    std::string                  url_;
    std::unique_ptr<std::string> data_;

    static std::size_t callback(const char* in, std::size_t size, std::size_t num,
                                std::string* out) {
        const std::size_t total_size = size * num;
        out->append(in, total_size);
        return total_size;
    }

    std::wstring get_icon_for_id(int32_t id) {
        (void)id;

        if (199 < id && id < 300) {
            // thunderstorm
            if (id < 210) {
                // thunkderstorm with rain
                return L"\uf01e";
            } else

                if (id < 230) {
                // thunderstorm
                return L"\uf016";
            } else {
                // thunderstorm with drizzle
                return L"\uf01d";
            }
        } else if (id < 300) {
            // drizzle
            return L"\uf01a";
        } else if (499 < id && id < 600) {
            // rain
            if (id < 510) {
                // regular rain
                return L"\uf019";
            } else if (id == 511) {
                // freezing rain
                return L"\uf017";
            } else {
                // severe rain
                return L"\uf018";
            }
        } else if (id < 700) {
            // snow
            return L"\uf01b";
        } else if (id < 800) {
            // atmosphere
            return L"\uf063";
        } else if (800 < id && id < 900) {
            return L"\uf013";
        } else {
            std::time_t time_temp = std::time(nullptr);
            std::tm*    time_out  = std::localtime(&time_temp);
            if (time_out->tm_hour < 6 && time_out->tm_hour < 7) {
                // clear day
                return L"\uf00d";
            } else {
                // clear night
                return L"\uf02e";
            }
        }

        return L"";
    }
};

int main(void) {
    // auto paper = Epaper();

    // paper.SetUpIos();

    // paper.InitFullUpdate();
    // paper.ClearDisplay();
    // paper.Wait(200);

    // auto image = Bitmap(Epaper::kHeight, Epaper::kWidth);

    // auto weather_renderer = TextRenderer(65, TextRenderer::Fonts::kWeather);
    // auto text_renderer    = TextRenderer(44, TextRenderer::Fonts::kLetterBoard);

    // auto weather = weather_renderer.RenderText(L"\uf00d");
    // weather_renderer.DrawOnImage(image, weather, 0, 0);

    // auto time = text_renderer.RenderText(get_timestring());
    // weather_renderer.DrawOnImage(image, time, 0, Epaper::kWidth - time.width());

    // auto description = text_renderer.RenderText(L"-28° CLOUDY");
    // weather_renderer.DrawOnImage(image, description, weather.height(), 0);

    // auto smaller_weather_renderer = TextRenderer(37, TextRenderer::Fonts::kWeather);
    // auto text_lower               = text_renderer.RenderText(L"-25");
    // auto text_lower_icon          = smaller_weather_renderer.RenderText(L"\uf018");

    // auto offset = weather.height() + 4;
    // weather_renderer.DrawOnImage(image, text_lower_icon, offset, description.width() + 12);

    // offset += text_lower_icon.height() + 4;
    // weather_renderer.DrawOnImage(image, text_lower, offset, description.width() + 12);

    // offset += text_lower.height() + 4;
    // weather_renderer.DrawOnImage(image, text_lower_icon, offset, description.width() + 12);

    // offset += text_lower_icon.height() + 4;
    // weather_renderer.DrawOnImage(image, text_lower, offset, description.width() + 12);

    // image.Print();

    // paper.DisplayImage(image.Raw());

    // std::wcout << "Press <Enter> to continue..." << std::endl;
    // std::cin.get();

    // paper.ClearDisplay();
    // paper.Shutdown();

    Weather weather("ll");

    auto forecast = weather.GetForecast();

    std::wcout << forecast->description << std::endl;
    std::wcout << int(forecast->icon.c_str()[0]) << std::endl;

    for (int i = 0; i < 4; i++) {
        std::wcout << forecast->temperatures[i] << std::endl;
    }

    return 0;
}