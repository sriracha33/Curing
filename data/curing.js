var refresh_rate = 2000;
var temperature;
var temperatureSP;
var tempControl;
var tempOn;
var humidity;
var humiditySP;
var humidityControl;
var humidityOn;

var xmlpath="/data.xml"
var commandpath="/command.cgi"

//run once at startup
$(document).ready(xmlRequest = function (){
    $("#tempControl").click(function (){$.post(commandpath,{command:1,value:!tempControl ? 1:0});}).css("cursor", "pointer");
    $("#humidityControl").click(function (){$.post(commandpath,{command:3,value:!humidityControl ? 1:0});}).css("cursor", "pointer");
    $(".changetemp").click(function(){askSetpoint(2)}).css("cursor", "pointer");
    $(".changehumidity").click(function(){askSetpoint(4)}).css("cursor", "pointer");
});
   
//recurring function to update at refresh_rate(ms)
$(document).ready(xmlRequest = function (){
	$.get(xmlpath, function(data, status){xmlReceived(data)}).fail(function(){setTimeout(function(){xmlRequest()},refresh_rate);});
});
               
function askSetpoint(command){
    //request response
    var value = window.prompt("Enter new setpoint");
    
    //return if no response
    if (value==null){
        return;
    }
    //convert response to int
    value=(parseInt(value));
    
    //return if no valid int.
    if (isNaN(value)){
        window.alert('Invalid entry, not a number')
        return;    
    }
    
    $.post(commandpath,{command:command,value:value});
    console.log(value)
}

function xmlReceived(data){
    temperature = data.getElementsByTagName('temperature')[0].childNodes[0].nodeValue;
    temperatureSP = data.getElementsByTagName('temperatureSP')[0].childNodes[0].nodeValue;
    tempControl = data.getElementsByTagName('tempControl')[0].childNodes[0].nodeValue;
    tempOn = data.getElementsByTagName('tempOn')[0].childNodes[0].nodeValue;
    tempOnPercent = data.getElementsByTagName('tempOnPercent')[0].childNodes[0].nodeValue;
    tempOnTimeAvg = data.getElementsByTagName('tempOnTimeAvg')[0].childNodes[0].nodeValue;
    humidity = data.getElementsByTagName('humidity')[0].childNodes[0].nodeValue;
    humiditySP = data.getElementsByTagName('humiditySP')[0].childNodes[0].nodeValue;
    humidityControl = data.getElementsByTagName('humidityControl')[0].childNodes[0].nodeValue;
    humidityOn = data.getElementsByTagName('humidityOn')[0].childNodes[0].nodeValue;
    humidityOnPercent = data.getElementsByTagName('humidityOnPercent')[0].childNodes[0].nodeValue;
    humidityOnTimeAvg = data.getElementsByTagName('humidityOnTimeAvg')[0].childNodes[0].nodeValue;
    
    temperature=parseFloat(temperature);
    temperatureSP=parseInt(temperatureSP);
    tempControl=!!parseInt(tempControl);
    tempOn=!!parseInt(tempOn);
    tempOnPercent=parseInt(tempOnPercent);
    tempOnTimeAvg=parseInt(tempOnTimeAvg);
    humidity=parseFloat(humidity);
    humiditySP=parseInt(humiditySP);
    humidityControl=!!parseInt(humidityControl);
    humidityOn=!!parseInt(humidityOn);
    humidityOnPercent=parseInt(humidityOnPercent);
    humidityOnTimeAvg=parseInt(humidityOnTimeAvg);
    
    
    $("#temperature").html(temperature.toFixed(1));
    $("#temperatureSP").html(temperatureSP);
    $("#tempControl").html(tempControl ? "On":"Off");
    $("#tempOn").html(tempOn ? "1":"0");
    $("#tempOnPercent").html(tempOnPercent);
    $("#tempOnTimeAvg").html(tempOnTimeAvg);
    $("#humidity").html(humidity.toFixed(1));
    $("#humiditySP").html(humiditySP);
    $("#humidityControl").html(humidityControl ? "On":"Off");
    $("#humidityOn").html(humidityOn ? "1":"0");
    $("#humidityOnPercent").html(humidityOnPercent);
    $("#humidityOnTimeAvg").html(humidityOnTimeAvg);
    
    setTimeout(function(){xmlRequest()},refresh_rate);
}












