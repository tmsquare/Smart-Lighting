// C++ raw string literals cf http://en.cppreference.com/w/cpp/language/string_literal
// USE PROGMEM with Program Space Utilities http://www.nongnu.org/avr-libc/user-manual/group__avr__pgmspace.html

const char troisieme[] PROGMEM = R"--8<--8<--(HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<!DOCTYPE HTML>
<html><head>
<meta charset="utf-8">
<style >
body {
  font-family: 'Open Sans', sans-serif;
  font-weight: 300;
  line-height: 1.42em;
  color:#A7A1AE;
  background-color:#1F2739;
}

h1 {
  font-size:3em; 
  font-weight: 300;
  line-height:1em;
  text-align: center;
  color: #4DC3FA;
}

h2 {
  font-size:1em; 
  font-weight: 300;
  text-align: center;
  display: block;
  line-height:1em;
  padding-bottom: 2em;
  color: #FB667A;
}

h2 a {
  font-weight: 700;
  text-transform: uppercase;
  color: #FB667A;
  text-decoration: none;
}

.blue { color: #185875; }
.yellow { color: #FFF842; }

.container th h1 {
    font-weight: bold;
    font-size: 1em;
  text-align: left;
  color: #185875;
}

.container td {
    font-weight: normal;
    font-size: 1em;
  -webkit-box-shadow: 0 2px 2px -2px #0E1119;
     -moz-box-shadow: 0 2px 2px -2px #0E1119;
          box-shadow: 0 2px 2px -2px #0E1119;
}

.container {
    text-align: left;
    overflow: hidden;
    width: 80%;
    margin: 0 auto;
  display: table;
  
}

.container td, .container th {
    padding-bottom: 2%;
    padding-top: 2%;
  padding-left:2%;  
}

/* Background-color of the odd rows */
.container tr:nth-child(odd) {
    background-color: #323C50;
}

/* Background-color of the even rows */
.container tr:nth-child(even) {
    background-color: #2C3446;
}

.container th {
    background-color: #1F2739;
}

.container td:first-child { color: #FB667A; }

.container tr:hover {
   background-color: #464A52;
-webkit-box-shadow: 0 6px 6px -6px #0E1119;
     -moz-box-shadow: 0 6px 6px -6px #0E1119;
          box-shadow: 0 6px 6px -6px #0E1119;
}

.container td:hover {
  background-color: #FFF842;
  color: #403E10;
  font-weight: bold;
  
  box-shadow: #7F7C21 -1px 1px, #7F7C21 -2px 2px, #7F7C21 -3px 3px, #7F7C21 -4px 4px, #7F7C21 -5px 5px, #7F7C21 -6px 6px;
  transform: translate3d(6px, -6px, 0);
  
  transition-delay: 0s;
    transition-duration: 0.4s;
    transition-property: all;
  transition-timing-function: line;
}

@media (max-width: 800px) {
.container td:nth-child(4),
.container th:nth-child(4) { display: none; }
}
</style>
    <title>ON</title>

</head>
<body>

<h1 align="center" ><span class="blue">&lt;</span>Smart<span class="blue">&gt;</span> <span class="yellow">Lighting</pan></h1>
    <h2 align="center"><b>Interface de visualisation</b></h2>
<table border="8" width=970 class="container" align="center" >
      <thead>
        <tr>
          <th><h5> <b>//</b> </h5></th>
          <th><h5> <b> LAMPE 1</b> </h5></th>
          <th><h5> <b> LAMPE 2</b> </h5></th>
          <th><h5> <b> LAMPE 3</b> </h5></th>
          <th><h5><b>LAMPE 4</b></h5></th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td> <b>LAMPES</b> </td>
          <td> <b> Fonctionnelle </b> </td>
          <td><b> Fonctionnelle </b></td>
          <td><b> Grille </b></td>
          <td><b> Fonctionnelle </b></td>
        </tr>
        <tr>
          <td> <b>Etats</b> </td>
          <td> Allumee </td>
          <td>Allumee</td>
          <td>Allumee</td>
          <td>Allumee</td>
        </tr>

      </tbody>
    </table> <br> <br>
    <button  onclick="location.href='/Allumer'" type='button'> Allumer  </button> <br> <br> <br>
    <button  onclick="location.href='/Eteindre'" type='button'> Eteindre  </button>


</body>
</html>
)--8<--8<--";
