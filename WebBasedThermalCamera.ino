/*
Started with https://github.com/acrobotic/Ai_Tips_ESP8266/tree/master/webserver_websockets

*/
#include <WiFi.h> // Part of ESP32 board libraries
#include <WebServer.h> // Part of ESP32 board libraries
#include <WebSocketsServer.h> // Installed "WebSockets" library
#include <Adafruit_AMG88xx.h>

Adafruit_AMG88xx amg;

float pixels[AMG88xx_PIXEL_ARRAY_SIZE];

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

char* ssid = "wifi_ssid_to_connect_to";
char* password = "wifi_password";

char webpage[] PROGMEM = R"=====(
<html>
<head>
	<script>
		var Socket;
		var sizesensor = [8,8];
		var minTsensor = 0.0;
		var maxTsensor = 80.0;

		var minT = 15.0; // initial low range of the sensor (this will be blue on the screen)
		var maxT = 50.0; // initial high range of the sensor (this will be red on the screen)

		var currentFrame = [];
		var minTframe = maxTsensor;
		var maxTframe = minTsensor;
		var minTframelocation = [0,0];
		var maxTframelocation = [0,0];

		var camColors = [
		"1500BF", "0900BF", "0003C0", "0010C0", "001DC1", "002AC2", "0037C2", "0044C3",
		"0052C3", "005FC4", "006DC5", "007AC5", "0088C6", "0095C6", "00A3C7", "00B1C8",
		"00BFC8", "00C9C4", "00C9B8", "00CAAB", "00CB9E", "00CB90", "00CC83", "00CC76",
		"00CD68", "00CE5B", "00CE4D", "00CF40", "00CF32", "00D024", "00D116", "00D109",
		"05D200", "13D200", "21D300", "2FD400", "3DD400", "4CD500", "5AD500", "69D600",
		"78D700", "86D700", "95D800", "A4D800", "B3D900", "C2DA00", "D1DA00", "DBD500",
		"DBC700", "DCB900", "DDAA00", "DD9C00", "DE8E00", "DE7F00", "DF7100", "E06200",
		"E05300", "E14400", "E13500", "E22600", "E31700", "E30800", "E40006", "E50015",
		];

		function mapto( val, minin, maxin, minout ,maxout ){
			if ( val >= maxin ){
				return maxout;
			} else if ( val <= minin ){
				return minout;
			} else {
				return ((( ( maxout - minout )/( maxin - minin ) ) * val ) + ( minout - ( ( maxout - minout )/( maxin - minin ) ) * minin ));
			}
		}

		function adjusttemrange(range,adjustment){
			if ( range == "min" ){
				if ( adjustment == 0){
					minT = minTframe;
				} else {
					minT += adjustment;
				}
			}
			if ( range == "max" ){
				if ( adjustment == 0){
					maxT = maxTframe;
				} else {
					maxT += adjustment;
				}
			}
			if ( minT < minTsensor ){
				minT = minTsensor;
			}
			if ( maxT > maxTsensor ){
				maxT = maxTsensor;
			}
			if ( minT >= maxT ){
				minT = maxT - 1.0;
			}
			fillthermcamtable(currentFrame);
			document.getElementById("minT").innerHTML = minT.toFixed(2) + "C";
			document.getElementById("maxT").innerHTML = maxT.toFixed(2) + "C";
		}

		function ctof(tempc){
			return (tempc * ( 9 / 5 ) ) + 32;
		}

		function buildthermcamtable(){
			var thermcamTable = document.getElementById("thermcam");
			for(i=0;i<sizesensor[0];i++){
				var row = thermcamTable.insertRow(i);
				for(j=0;j<sizesensor[1];j++){
					var cell = row.insertCell(j);
					cell.id = "tccell"+((i*8)+j);
					cell.height = 30;
					cell.width = 60;
				}
			}
		}

		function fillthermcamtable(pixelarray){
			minTframe = maxTsensor;
			maxTframe = minTsensor;
			// Fill table
			// Temps in C and F in cells
			// BG color of cells use camColors
			// Update minTframe and maxTframe
			for(i=0;i<sizesensor[0];i++){
				for(j=0;j<sizesensor[1];j++){
					var idx = ((i*8)+j);
					var cell = document.getElementById("tccell"+idx);
					var camcolorvalue = Math.round( mapto( pixelarray[idx], minT , maxT, 0 ,63 ) );
					cell.innerHTML = pixelarray[idx].toFixed(2) + "C " + ctof( pixelarray[idx] ).toFixed(2) + "F";
					cell.style.backgroundColor = "#" + camColors[camcolorvalue];
					cell.className = "camcolorvalue" + camcolorvalue;
					if ( pixelarray[idx] < minTframe ){
						minTframe = pixelarray[idx];
						minTframelocation = [j,i];
					}
					if ( pixelarray[idx] > maxTframe ){
						maxTframe = pixelarray[idx];
						maxTframelocation = [j,i];
					}
				}
			}
			document.getElementById("minTframedata").innerHTML = minTframe.toFixed(2) + "C " + ctof( minTframe ).toFixed(2) + "F (" + minTframelocation[0] + "," + minTframelocation[1] + ")";
			document.getElementById("maxTframedata").innerHTML = maxTframe.toFixed(2) + "C " + ctof( maxTframe ).toFixed(2) + "F (" + maxTframelocation[0] + "," + maxTframelocation[1] + ")";
		}

		function init() {
			// Build table
			buildthermcamtable();

			Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
			Socket.onmessage = function(event){
				document.getElementById("rxConsole").value += event.data;
				if ( event.data[0] == '[' ){
					currentFrame = JSON.parse(event.data);
					fillthermcamtable(currentFrame);
				}
			}
		}

		function sendText(){
			Socket.send(document.getElementById("txBar").value);
			document.getElementById("txBar").value = "";
		}

		function getframe(){
			Socket.send("F");
		}

	</script>
</head>
<body onload="init()">
	<table id="thermcam">
	</table>
	<hr/>
	<table id="thermcamcontrols">
		<tr>
			<td>Min Temp</td>
			<td id="minT"></td>
			<td><button type="button" onclick="adjusttemrange('min',-5);">-5C</button></td>
			<td><button type="button" onclick="adjusttemrange('min',-1);">-1C</button></td>
			<td><button type="button" onclick="adjusttemrange('min', 0);">lowest in current frame</button></td>
			<td><button type="button" onclick="adjusttemrange('min',+1);">+1C</button></td>
			<td><button type="button" onclick="adjusttemrange('min',+5);">+5C</button></td>
			<td id="minTframedata"></td>
		</tr>
		<tr>
			<td>Max Temp</td>
			<td id="maxT"></td>
			<td><button type="button" onclick="adjusttemrange('max',-5);">-5C</button></td>
			<td><button type="button" onclick="adjusttemrange('max',-1);">-1C</button></td>
			<td><button type="button" onclick="adjusttemrange('max', 0);">highest in current frame</button></td>
			<td><button type="button" onclick="adjusttemrange('max',+1);">+1C</button></td>
			<td><button type="button" onclick="adjusttemrange('max',+5);">+5C</button></td>
			<td id="maxTframedata"></td>
		</tr>
	</table>
	<hr/>
	<div>
		<button type="button" onclick="getframe();">Get Frame</button>
	</div>
	<hr/>
	<div>
		<textarea id="rxConsole"></textarea>
	</div>
	<hr/>
	<div>
		<input type="text" id="txBar" onkeydown="if(event.keyCode == 13) sendText();" />
	</div>
</body>
</html>
)=====";

void setup()
{
  WiFi.begin(ssid,password);
  Serial.begin(115200);
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/",[](){
    server.send_P(200, "text/html", webpage);  
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Wire.begin(13,15); // AMG8833 sensor usin 13 as SDA and 15 as SCL
 if (!amg.begin()) {
    Serial.println("Could not find a valid AMG88xx sensor, check wiring!");
    while (1);
  }
}

void loop()
{
  webSocket.loop();
  server.handleClient();
  if(Serial.available() > 0){
    char c[] = {(char)Serial.read()};
    webSocket.broadcastTXT(c, sizeof(c));
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  if(type == WStype_TEXT){
    if(payload[0] == 'F'){ // i.e. "Frame"
		amg.readPixels(pixels);

		String thermalFrame = "[";

		for (int i = 1; i < AMG88xx_PIXEL_ARRAY_SIZE + 1; i++) { // Starts at 1 so that (i % 8) will be 0 at end of 8 pixels and newline will be added
			thermalFrame += pixels[i - 1];
			if ( i != AMG88xx_PIXEL_ARRAY_SIZE ){
				thermalFrame += ", ";
				if ( i % 8 == 0 ) thermalFrame += "\n";
			}
		}
		thermalFrame += "]";

		char tf[thermalFrame.length()+1];

		thermalFrame.toCharArray( tf, thermalFrame.length()+1 );

		Serial.println(tf);

		webSocket.broadcastTXT(tf, sizeof(tf)-1);

    } else{
      for(int i = 0; i < length; i++)
        Serial.print((char) payload[i]);
      Serial.println();
    }
  }
  
}
