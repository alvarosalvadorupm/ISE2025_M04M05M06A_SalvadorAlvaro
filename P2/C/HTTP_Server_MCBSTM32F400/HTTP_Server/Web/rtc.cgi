t <html><head><title>Time/Date</title>
t <script language=JavaScript type="text/javascript" src="xml_http.js"></script>
t <script language=JavaScript type="text/javascript">
# Define URL and refresh timeout
t var formUpdate = new periodicObj("rtc.cgx", 1000);
t function periodicUpdateAd() {
t  if(document.getElementById("adChkBox").checked == true) {
t   updateMultiple(formUpdate,plotADGraph);
t   ad_elTime = setTimeout(periodicUpdateAd, formUpdate.period);
t  }
t  else
t   clearTimeout(ad_elTime);
t }
t </script></head>
i pg_header.inc
t <h2 align="center"><br>Hora y Fecha Actuales</h2>
t <p><font size="2">Esta pagina permite alusuario tanto visulizar como modificar
t  los valores de la hora y la fecha con los que cuenta nuestro RTC. Si decidimos
t  clikear en la opci�n <b> Periodic </b> iremos refrescando los valores de la hora
t  y la fecha.</font></p>
t <form action="rtc.cgi" method="post" name="rtc">
t <input type="hidden" value="rtc" name="pg">
t <table border=0 width=99%><font size="3">
t <tr style="background-color: #aaccff">
t  <th width=40%>Item</th>
t  <th width=60%>Values</th>
# Here begin data setting which is formatted in HTTP_CGI.C module
t <tr><td><img src=pabb.gif>Hora:</td>
c h 1 <td><input type=text name=hour size=20 maxlength=20 value="%s"></td></tr>
t <tr><td><img src=pabb.gif>Fecha:</TD>
c h 2 <td><input type=text name=date size=20 maxlength=20 value="%s"></td></tr>
t </font></table>
t <p align=center>
t <input type=submit name=set value="Send" id="sbm">
t Periodic:<input type="checkbox" id="adChkBox" onclick="periodicUpdateAd()">
t </p></form>
i pg_footer.inc
. End of script must be closed with period
