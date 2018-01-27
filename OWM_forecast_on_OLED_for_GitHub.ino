/*
 *	Работа с API openweathermap.org.
 *
 *
 *
 *
 *
 *
 */

/*-------------справочные данные----------------
  //шаблон GET-запроса - api.openweathermap.org/data/2.5/forecast?q={city name},{country code}
  //api.openweathermap.org/data/2.5/forecast?id=1490624&appid=
  //id г. Сургута в OWM - 1490624
  ----------------------------------------------*/

#include <ESP8266WiFi.h>            			//методы для подключения к сети WiFi
#include <ESP8266HTTPClient.h>      			//методы для отправки HTTP-запросы
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
//#define ARDUINO_DEFAULT_NESTING_LIMIT  300;   //глубина вложенности элементов в массиве
#define TIME_ZONE 5								//определяем здесь часовой пояс
#define LED_BUILTIN  2       					//встроенный светодиод платы ESP8266 "висит" на GPIO2, эта цепь инвертирована - подтянута к питанию, для включения - необходимо подавать лог 0 на неё!
#define OLED_RESET 2
#define APPID "23c44e5e16a618179ddb96457d819225"
#define cityID 1490624

Adafruit_SSD1306 display(OLED_RESET);			//объявление экземпляра класса OLED-дисплея
												//прототипы функции
String request(void);             				//функция формирования HTTP-запроса
void JS_Parse(String);							//функция дешифровки JSON-строки
String Epoch_Time_Convert(int);					//функция вычисления времени из Unixtimestamp

const char* ssid = "RTK-402318";				//данные для подключения к Wi-Fi
const char* password = "ELTX5C0245F0";			//
// const char* ssid = "AAAAAEx47B4AAwHSRedmi";	//
// const char* password = "C6360436";			//

/*
#
#
#---------------------[функция setup]
#
#
*/
void setup(void)
{
	WiFi.begin(ssid, password);         			//подключаемся к местному WiFi
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  	//инициализация OLED-дисплея на I2C-шине по адресу 0x3C (адрес для дисплеев разрешением 128x32)
	display.clearDisplay();							//очистка дисплея
	Serial.begin(115200);               			//открытие Serial-порта
	pinMode(LED_BUILTIN, OUTPUT);					//настройка пина как выход
	delay(1000);
	
	Serial.print("Подключение");
	while (WiFi.status() != WL_CONNECTED)			//циклические попытки подключиться к WiFi
	{
		delay(500);                       			//ожидание успешного подключения к WiFi
		Serial.print(".");
	}
	Serial.println();
	Serial.println("Подключение выполнено!");
	
	display.setTextSize(1);							//настройки текстового вывода OLED-дисплея
	display.setTextColor(WHITE);
}
/*
#
#
#		функция loop
#
#
*/
void loop(void)
{
	display.clearDisplay();							//очистка дисплея
	String rabota;              					//переменная, в которую записываем ответ сервера OWM

	rabota = request();         					//выполняем GET-запрос, возвращаем результат в переменную rabota
	JS_Parse(rabota);           					//вызываем функцию парсинга JSON
	
	display.drawPixel(125, 30, WHITE);				//значок, что устройство дошло до команды deepSleep
	display.display();
	ESP.deepSleep(3600e6); 							//60 минут (3600 * 10^6 мкс) в режиме "deep sleep" 
}
/*
#
#
#		функция request
#
#
*/
String request(void)
{
  int httpcode;                 					//локальная переменная, содержащая статус ответа сервера
  String response;
  String zapros;
  
  zapros = "http://api.openweathermap.org/data/2.5/forecast?id=" cityID "&appid=" APPID;

  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;            					//объявляем экземпляр http-класса
    Serial.println("Выполнение GET-запроса");
    //формируем HTTP-запрос
    http.begin(zapros);
    // http.begin("http://api.openweathermap.org/data/2.5/weather?id=1490624&appid=23c44e5e16a618179ddb96457d819225");
    http.addHeader("Content-Type", "application/json");
    httpcode = http.GET();              			//код ответа от сервера (200 -- успех!)
    response = http.getString();        			//в response записываем ответ от сервера
    http.end();                         			//заканчиваем работу с http
    return response;
  }
  else
  {
    Serial.println("Нет связи!");
    return "error";
  }
}
/*
#
#
#		функция JS_Parse
#
#
*/
void JS_Parse(String parsthis)
{
	String pars_begin = "\"list\":[{";  			//строковые переменные, включающие символы начала и конца массива данных JSON
	String pars_end = ",{";             			//строковые переменные, включающие символы начала и конца массива данных JSON
	String resp_substring;        					//переменная, содержащая только массив JSON-данных {...}

	int otrezok_begin = 0;        					//индекс стартового элемента JSON-строки
	int otrezok_end = 0;          					//индекс конечного элемента JSON-строки
	int j = 0;            							//счетчики цикла
	
	float temperature[8];							//массив значений температуры
	float pressure[8];        						//массив значений давления
	int humidity[8];         						//массив значений влажности
	int dt[8];             							//массив временных отметок в формате UNIX-time
	// String description[8];						//массив строк, попробуем описание погоды затолкать в него...

	otrezok_begin = parsthis.indexOf(pars_begin);   //ищем в исходной JSON-строке место начала

	//это для проверки работы программы
	// Serial.print("искомая строка найдена, индекс первого символа = ");
	// Serial.println(otrezok_begin); //выводит на печать 39

	otrezok_begin += 8;       						//смещаемся на 8 символов далее по строке -- там будет находиться начало массива данных JSON

	for(j = 0; j < 8; j++)							//в цикле заполняем массивы данных
	{
		
		otrezok_end = parsthis.indexOf(pars_end, otrezok_begin);  //ищем в исходной JSON-строке символы ",{"
		resp_substring = parsthis.substring(otrezok_begin, otrezok_end);

		//			сюда вписываем код JSON-парсера. Пусть работает над полученной строкой.
		//
		//
		DynamicJsonBuffer jsonBuffer(900);
		JsonObject& root = jsonBuffer.parseObject(resp_substring);
		if (!root.success())
		{
			Serial.println("парсинг - ОТКАЗ!");
			return;
		}
		else
		{
			JsonObject& main = root["main"];
			
			dt[j] = root["dt"];
			
			float main_temp = main["temp"];
			temperature[j] = main_temp - 273.0;

			float press = main["pressure"];
			pressure[j] = press * 0.75;		//перевожу давление в мм.рт.ст. (mmHg)
			humidity[j] = main["humidity"]; 
			// description[j] = String(root["weather"][0]["description"]);
		} 
		otrezok_begin = otrezok_end + 1;
	}
	//красивый вывод полученных данных
	Serial.println("\tПолученные данные.");
	Serial.print("Дата\t");
	Serial.print("Описание\t");
	Serial.print("Температура\t");
	Serial.print("Давление\t");
	Serial.print("Влажность\t");
	Serial.println();
	for(j = 0; j < 8; j++)
	{
		Serial.print(Epoch_Time_Convert(dt[j]));
		Serial.print("\t");
		Serial.print(dt[j]);
		Serial.print("\t");
		Serial.print(temperature[j]);
		Serial.print("\t");
		Serial.print(pressure[j]);
		Serial.print("\t");
		Serial.print(humidity[j]);
		Serial.print("\t");
		Serial.println();
	}
	display.setCursor(0, 0);
	display.println(Epoch_Time_Convert(dt[0]));
	display.setCursor(0, 8);
	display.println(temperature[0]);
	display.setCursor(0, 16);
	display.println(pressure[0]);
	display.setCursor(0, 24);
	display.println(humidity[0]);
	
	display.setCursor(40, 0);
	display.println(Epoch_Time_Convert(dt[1]));
	display.setCursor(40, 8);
	display.println(temperature[1]);
	display.setCursor(40, 16);
	display.println(pressure[1]);
	display.setCursor(40, 24);
	display.println(humidity[1]);
	
	display.setCursor(80, 0);
	display.println(Epoch_Time_Convert(dt[2]));
	display.setCursor(80, 8);
	display.println(temperature[2]);
	display.setCursor(80, 16);
	display.println(pressure[2]);
	display.setCursor(80, 24);
	display.println(humidity[2]);
	
	display.display();
}
/*
#
#
#		функция Epoch_Time_Convert
#
#
*/
String Epoch_Time_Convert(int epoch)
{
	String outtime;
	int h, m;
	h = ((epoch + TIME_ZONE * 3600)  % 86400L) / 3600;	// print the hour (86400 equals secs per day)
	outtime = String(h) + ':';
	if ( ((epoch % 3600) / 60) < 10 ) {
		outtime = outtime + '0';												// In the first 10 minutes of each hour, we'll want a leading '0'		
	}
	m = (epoch  % 3600) / 60;	// print the minute (3600 equals secs per minute)
	outtime = outtime + String(m);
	return outtime;
}
