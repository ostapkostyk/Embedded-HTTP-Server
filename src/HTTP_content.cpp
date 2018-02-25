/**
  ******************************************************************************
  * @file    HTTP_content.cpp
  * @author  Ostap Kostyk
  * @brief   Contain HTTP content for the HTTP server implemented in HTTP_Server
  *          class
  *
  ******************************************************************************
  * Copyright (C) 2018  Ostap Kostyk
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version provided that the redistributions
  * of source code must retain the above copyright notice.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  * Author contact information: Ostap Kostyk, email: ostap.kostyk@gmail.com
  ******************************************************************************
 */

#include "HTTP_content.h"

/* Tip: use on-line Text to C/C++ converter to convert your HTTP page to string */
/* Tip: scripts should be transmitted as one message, so better not to divide script into parts (separate strings) */

char HTTP_StringForRendering[HTML_RENDER_STR_SIZE];         //  string for dynamic content generation (optional)
char* pHTTP_StringForRendering = HTTP_StringForRendering;

char HTTP_StringForRendering2[HTML_RENDER_STR_SIZE];         //  string for dynamic content generation (optional)
char* pHTTP_StringForRendering2 = HTTP_StringForRendering2;

/*************************************************************************
 *          HEADER INFO
 *************************************************************************/

const char HTTP_Header[] =
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\n"
        "\"http://www.w3.org/TR/html4/strict.dtd\">\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">\n"
        "<title>Device</title>\n"
        "<style> \n"
        ".button {\n"
        "background-color: #4CAF50;\n"
        "border: yes;\n"
        "color: white;\n"
        "padding: 8px 32px;\n"
        "margin: 4px 2px;\n"
        "text-align: center;\n"
        "text-decoration: none;\n"
        "display: inline-block;\n"
        "font-size: 16px;\n"
        "cursor: pointer;\n"
        "}\n"
        "a.button {\n"
        "-webkit-appearance: button;\n"
        "-moz-appearance: button;\n"
        "appearance: button;\n"
        "text-decoration: none;\n"
        "display:block;\n"
        "font-size: 20px;\n"
        "}\n"
        ".button1 {background-color: #008CBA;} /* Blue */\n"
        "canvas {border: 3px #CCC solid;}\n"
        "</style>\n"
        "</head>\n";

/*************************************************************************
 *          INDEX PAGE
 *************************************************************************/

const char HTTP_Index_Body1[] =
        "<body>\n"
        "<div>\n"
        "<table style=\"height: 280px; width: 100%;\">\n"
        "<tbody>\n"
        "<tr style=\"height: 43px;\">\n"
        "<td style=\"width: 80%; height: 43px;\" colspan=\"3\" bgcolor=\"green\">\n"
        "<h1 style=\"padding-left: 30px;\">Device Control and Configuration</h1>\n"
        "</td>\n"
        "</tr>\n"
        "<tr style=\"height: 229.8px; vertical-align: top;\">\n"
        "<td style=\"width: 20%; height: 400px;  text-align: center;\" bgcolor=\"#8FBC8F\">\n"
        "<a href=\"/\" class=\"button\"><div style=\"width:95%\">Home</div></a>\n"
        "<a href=\"settings.html\" class=\"button\"><div style=\"width:95%\">Settings</div></a>\n"
        "</td>\n"
        "<td style=\"width: 50%; height: 400px;  text-align: center;\" bgcolor=\"#F5F5DC\">\n"
        "<h2>Control</h2>\n"
        "<p style=\"text-align: left;\"><b>Blue LED:</b><br />State:\n"
        "<div id=\"container\">\n"
        "<canvas id=\"BlueLEDCanvas\" height=\"100\" width=\"100\"></canvas>\n"
        "</div>\n"
        "<p style=\"text-align: left;\">Control:\n"
        "<form action=\"index.html\" method=\"get\">\n"
        "<input type=\"submit\" class=\"button button1\" name=\"BlueLEDMode\" value=\"ON\">\n"
        "<input type=\"submit\" class=\"button button1\" name=\"BlueLEDMode\" value=\"OFF\">\n"
        "<input type=\"submit\" class=\"button button1\" name=\"BlueLEDMode\" value=\"BLINK\">\n"
        "</form>\n"
        "<p id=\"BLEDMode\", visibility: hidden>";

// dynamic part of index.html page
char HTTP_Index_Body2[] = "0";

// static part of index.html page
const char HTTP_Index_Body3[] =
        "</p>"
        "</p>\n"
        "</td>\n"
        "<td style=\"width: 30%; height: 400px;\" bgcolor=\"#DCDCDC\"><p>Info:</p><p>Use Buttons to control LEDs modes<br /><br />Gray color of the circle means that the LED is off</p></td>\n"
        "</tr>\n"
        "<tr style=\"height: 20px;\">\n"
        "<td style=\"width: 94%; height: 40px; text-align: right;\" colspan=\"3\" bgcolor=\"#DCDCDC\">Developed by Ostap Kostyk&nbsp;&copy;</td>\n"
        "</tr>\n"
        "</tbody>\n"
        "</table>\n"
        "</div>\n"
        "<script>\n"
        "var mainCanvas = document.querySelector(\"#BlueLEDCanvas\");\n"
        "var mainContext = mainCanvas.getContext(\"2d\");\n"
        "var canvasWidth = mainCanvas.width;\n"
        "var canvasHeight = mainCanvas.height;\n"
        "var requestAnimationFrame = window.requestAnimationFrame || \n"
        "window.mozRequestAnimationFrame || \n"
        "window.webkitRequestAnimationFrame || \n"
        "window.msRequestAnimationFrame;\n"
        "var radius = 45;\n"
        "function drawCircle() {\n"
        "if (typeof drawCircle.BlinkCounter == 'undefined')\n"
        "{drawCircle.BlinkCounter = 0; }\n"
        "var Bcolor = \"#000000\";\n"
        "var LEDMode_elem = document.getElementById(\"BLEDMode\");\n"
        "var BlueLEDMode = LEDMode_elem.innerHTML;\n"
        "    mainContext.clearRect(0, 0, canvasWidth, canvasHeight);\n"
        "    // color in the background\n"
        "    mainContext.fillStyle = \"#EEEEEE\";\n"
        "    mainContext.fillRect(0, 0, canvasWidth, canvasHeight); \n"
        "    // draw the circle\n"
        "    mainContext.beginPath();\n"
        "    mainContext.arc(50, 50, radius, 0, Math.PI * 2, false);\n"
        "    mainContext.closePath();\n"
        "    // color in the circle\n"
        "    drawCircle.BlinkCounter++;\n"
        "\t\n"
        "\tswitch(BlueLEDMode)\n"
        "\t{\n"
        "\tcase \"0\": Bcolor = \"#B8B8B8\"; break;\n"
        "\tcase \"1\": Bcolor = \"#006699\"; break;\n"
        "\tcase \"2\": if(drawCircle.BlinkCounter < 20) { Bcolor = \"#B8B8B8\"; }\n"
        "    else if (drawCircle.BlinkCounter < 40) { Bcolor = \"#006699\"; }\n"
        "    else { drawCircle.BlinkCounter = 0; }\n"
        "\tbreak;\n"
        "\t}\n"
        "\tmainContext.fillStyle = Bcolor;\n"
        "    mainContext.fill();\n"
        "    window.requestAnimationFrame(drawCircle);\n"
        "}"
        "drawCircle(); \n"
        "</script>\n"
        "</body>\n"
        "</html>";

/*************************************************************************
 *          SETTINGS PAGE
 *************************************************************************/

// static part of settings.html page
const char HTTP_Settings_Body1[] =
        "<body onload=\"formChanged()\">\n"
        "<div>\n"
        "<table style=\"height: 280px; width: 100%;\">\n"
        "<tbody>\n"
        "<tr style=\"height: 43px;\">\n"
        "<td style=\"width: 80%; height: 43px;\" colspan=\"3\" bgcolor=\"green\">\n"
        "<h1 style=\"padding-left: 30px;\">Device Control and Configuration</h1>\n"
        "</td>\n"
        "</tr>\n"
        "<tr style=\"height: 229.8px; vertical-align: top;\">\n"
        "<td style=\"width: 20%; height: 400px;  text-align: center;\" bgcolor=\"#8FBC8F\">\n"
        "<a href=\"/\" class=\"button\"><div style=\"width:95%\">Home</div></a>\n"
        "<a href=\"settings.html\" class=\"button\"><div style=\"width:95%\">Settings</div></a>\n"
        "</td>\n"
        "<td style=\"width: 50%; height: 400px;  text-align: center;\" bgcolor=\"#F5F5DC\">\n"
        "<h2>Settings</h2>\n"
        "<p style=\"text-align: left;\"><b>Blue LED:</b>\n"
        "<form action=\"settings.html\" method=\"get\">\n"
        "<p style=\"text-align: left;\">\n"
        "LED On time: <input type=\"number\" style=\"width: 80px;\" id=\"bLEDOn\" name=\"BlueLEDBlinkTimeOn\" value=\"100\"> ms<br />\n"
        "LED Off time: <input type=\"number\" style=\"width: 80px;\" id=\"bLEDOff\" name=\"BlueLEDBlinkTimeOff\" value=\"200\"> ms<br />\n"
        "<p style=\"text-align: center;\">\n"
        "<input type=\"submit\" class=\"button button1\" value=\"Save\">\n"
        "</form>\n"
        "<hr>\n"
        "<p style=\"text-align: left;\"><b>WiFi SSID:</b><br /><br />";

// dynamic part of settings.html page
const char* pHTTP_Settings_Body2 = HTTP_StringForRendering2;

// static part of settings.html page
const char HTTP_Settings_Body3[] =
        "<form action=\"settings.html\" method=\"get\">\n"
        "<p style=\"text-align: left;\">\n"
        "New SSID (max 20 symbols): <input type=\"text\" name=\"WiFiSSID\"><br>\n"
        "<p style=\"text-align: center;\">\n"
        "<input type=\"submit\" class=\"button button1\" value=\"Submit\">\n"
        "</form>\n"
        "<p style=\"text-align: left;\">Note: device must be restarted to make the changes of SSID take effect.</p>"
        "</p>\n"
        "</td>\n"
        "<td style=\"width: 30%; height: 400px;\" bgcolor=\"#DCDCDC\"><p>Info:</p><p>LED On and Off times for blinking mode can be configured here.<br /> <br /> Time intervals are in milliseconds"
        "<br /> <br />SSID accepts symbols a-z, A-Z, numbers 0-9, minus and underscore"
        "</p></td>\n"
        "</tr>\n"
        "<tr style=\"height: 20px;\">\n"
        "<td style=\"width: 94%; height: 40px; text-align: right;\" colspan=\"3\" bgcolor=\"#DCDCDC\">Developed by Ostap Kostyk&nbsp;&copy;</td>\n"
        "</tr>\n"
        "</tbody>\n"
        "</table>\n"
        "</div>";

// dynamic part of settings.html page
const char* pHTTP_Settings_Body4 = HTTP_StringForRendering;

// static part of settings.html page
const char HTTP_Settings_Body5[] =
        "<script>\n"
        "function formChanged() {\n"
        "document.getElementById(\"bLEDOn\").defaultValue = document.getElementById(\"BLEDOnAct\").innerHTML;\n"
        "document.getElementById(\"bLEDOff\").defaultValue = document.getElementById(\"BLEDOffAct\").innerHTML;\n"
        "}\n"
        "window.onload = formChanged();\n"
        "</script>\n"
        "</body>\n"
        "</html>";

HTTP_Page IndexPage[] = {{HTTP_Header, sizeof(HTTP_Header)}, {HTTP_Index_Body1, sizeof(HTTP_Index_Body1)}, {HTTP_Index_Body2, 0}, {HTTP_Index_Body3, sizeof(HTTP_Index_Body3)}};
HTTP_Page SettingsPage[] = {{HTTP_Header, sizeof(HTTP_Header)}, {HTTP_Settings_Body1, sizeof(HTTP_Settings_Body1)}, {pHTTP_Settings_Body2, 0},
                            {HTTP_Settings_Body3, sizeof(HTTP_Settings_Body3)}, {pHTTP_Settings_Body4, 0}, {HTTP_Settings_Body5, sizeof(HTTP_Settings_Body5)}};

/* ====== Array of pages. First page must be home page. Last record in array must be zeros ======= */
HTTPServerContent_t HTTPServerContent[] = {
/* |---------------|---------------------------------------------------|--------------------------------------------|------------------------------------*/
/* |  *pPage       |   PageParts                                       |   *pPageName                               |   HTTP_PageType                    */
/* |---------------|---------------------------------------------------|--------------------------------------------|------------------------------------*/
/* | Pointer on    |   sizeof(<ArrayName>) / sizeof(HTTP_Page)         | Pointer or string                          |  Any value, field is filled by     */
/* |    Array      |                                                   | with page name                             |  application during initialization */
/* |---------------|---------------------------------------------------|--------------------------------------------|------------------------------------*/
   {   IndexPage,      sizeof(IndexPage) / sizeof(HTTP_Page),              "index.html",                                HTTP_PageType::Static        },
   {   SettingsPage,   sizeof(SettingsPage) / sizeof(HTTP_Page),           "settings.html",                             HTTP_PageType::Static        },
/* |---------------|---------------------------------------------------|--------------------------------------------|------------------------------------*/
                                                               /* ENDING ELEMENT, DO NOT CHANGE!!! */
   {   0,              0,                                                  0,                                           HTTP_PageType::Static        }
};

/*****************************************************************************************************************************
 *                                  VARIABLES
 *****************************************************************************************************************************/
HTTPVariable HTTP_VAR_BlueLEDMode("BlueLEDMode", HTTPVariable::HTTPVarType::Text, 20);
HTTPVariable HTTP_VAR_BlueLEDBlinkTimeOn("BlueLEDBlinkTimeOn", HTTPVariable::HTTPVarType::Integer);
HTTPVariable HTTP_VAR_BlueLEDBlinkTimeOff("BlueLEDBlinkTimeOff", HTTPVariable::HTTPVarType::Integer);
HTTPVariable HTTP_VAR_WiFiSSID("WiFiSSID", HTTPVariable::HTTPVarType::Text, 22);

