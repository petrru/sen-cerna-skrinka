Úvod
====

Cílem tohoto projektu je vytvořit jednoduchou "černou skříňku" pro automobil, která
v případě detekce nárazu odešle prostřednictvím Bluetooth informaci o poloze vozidla
a síle nárazu na server. Na serveru bude spuštěna aplikace, která tyto získané přijaté údaje
zobrazí.

Použité zařízení a technologie
==============================

Vlastní černá skříňka je realizována kitem [STM32 Nucleo 446RE][nucleo], který složí jako mikrokontrolér,
senzory pro zjištění vstupních veličin (zrychlení a poloha) a Bluetooth modulem pro komunikaci se serverem.
Konkrétně se jedná o akcelerometr
[Pololu LSM303D][acc], GPS senzor [u-blox NEO 6M][gps] a Bluetooth modul [Keyestudio Ks0055 HC-06][bt].
Ke komunikaci mezi kitem a akcelerometrem se používá rozhraní I2C, zatímco GPS a Bluetooth moduly
s kitem komunikují pomocí sériového rozhraní.

Serverová část je řešena jako aplikace pro Android, která může pomocí Bluetooth jednoduše
komunikovat s kitem dle zpráv popsaných v kapitole [#msgs].
<!--Server byl realizován aplikací pro mobilní zařízení se systémem Android. Tento způsob
byl vybrán především kvůli jednoduchosti demonstrace aplikace, velké dostupnosti telefonů
s Bluetooth konektivitou a snadné implementaci.-->

[nucleo]: https://www.st.com/en/evaluation-tools/nucleo-f446re.html
[acc]: https://www.pololu.com/product/2127
[gps]: https://www.u-blox.com/en/product/neo-6-series
[bt]: http://wiki.keyestudio.com/index.php/Ks0055_keyestudio_Bluetooth_Module

Schéma připojení periferií ke kitu
==================================

Na následujícím obrázku je znázorněno schéma zapojení periferií ke kitu STM32 Nucleo.
Všechna zařízení jsou napájena napětím 3,3 V a připojena k zemi. Bluetooth modul
je připojen pomocí sériové linky na UART1 (piny D2 a D8). Pro obsloužení GPS modulu
postačuje pouze jednosměrná komunikace směrem do kitu, stačí tedy připojit
výstupní vodič senzoru k vstupnímu pinu UART4 A1. Vodiče akcelerometru SDA a SCL
budou připojeny k pinům kitu D14 a D15 a dále je nutné na vstup akcelerometru SDO přivést
logickou 0, čímž bude povolena komunikace pomocí I2C.

![Schéma připojení periferních zařízení, 110%](schema.svg)

Komunikace mezi kitem STM32 Nucleo a serverem #msgs
===================================================

K přenosu informací mezi kitem STM32 Nucleo a serverem se využívá bezdrátová technologie
Bluetooth. K úspěšnému navázání spojení je nejprve nutné se prostřednictvím serverové aplikace
spárovat a připojit k černé skříňce. Jakmile je spojení vytvořeno, bude černá skříňka automatiky
odesílat zprávy o detekovaných nárazech na server (viz podkapitola [#crash]).
Dále skříňka na vyžádání serveru odešle svoji aktuální polohu ([#gps]) nebo
data z akcelerometru ([#acc]).

Zprávy zasílané černou skříňkou na server mají vždy délku jednoho řádku a jsou ukončeny
bajty `CR+LF`.
Pokyny odesílané serverem do skříňky obsahují vždy jen jeden bajt a neukončují se pomocí `CR+LF`.


Zpráva o detekovaném nárazu #crash
----------------------------------

Bude-li skříňkou detekován náraz, odešle černá skříňka bez jakéhokoliv vyžádání
ze strany serveru následující zprávu na server:

    crash;[acc-x];[acc-y];[acc-z];[lat];[lon]

Kde:

* `[acc-x]`, `[acc-y]` a `[acc-z]` jsou informace o síle nárazu v jednotlivých osách
    získané z akcelerometru. Síla nárazu v každé ose je vyjádřena číslem mezi -32768
    a 32767.
* `[lat]` a `[lon]` je zeměpisná šířka a délka místa, kde došlo k nárazu, ve formátu
    `[-]DDMMmmmmm`, kde `DD` jsou stupně, `MM` jsou minuty a `mmmmm` jsou stotisíciny minut.
    Např. hodnota `491394359` znamená 
    ```tex
    $49^\circ13,94359'$, tj. $49,232393^\circ$
    ```
    . Kladná hodnota znamená *north* nebo *east*, záporná hodnota reprezentuje
    *south* nebo *west*. Údaj `DD` se může skládat z 1-3 číslic, délka ostatních
    částí (`MM` a `mmmmmm`) je pevná. Není-li GPS poloha k dispozici, bude
    na místě obou složek odeslána hodnota `0`.

Příkladem konkrétní zprávy o nárazu odeslané na server může být:

    crash;31054;-724;19573;491394363;163523547


Získání aktuální polohy z GPS #gps
----------------------------------

Server může dále požádat černou skříňku o odeslání své aktuální polohy bez toho,
aby došlo k nárazu. Toto může být použito k ladícím účelům, lokalizaci ztraceného
vozidla či k zjištění polohy vozidla po nárazu v případě, že v době nárazu nebyla k dispozici
aktuální GPS poloha.

Pro vyžádání aktuální polohy odešle server skříňku zprávu obsahující jediný znak `g`.
Na tento požadavek skříňka odpoví následující zprávou:

    crash;[lat];[lon]
    
kde význam `[lat]` a `[lon]` je totožný jako u zprávy [#crash].

Získání aktuální hodnoty zrychlení #acc
---------------------------------------

Obdobným způsobem mohou být i vyčteny aktuální hodnoty z akcelerometru. Pro získání
těchto hodnot serverová aplikace odešle do černé skříňky zprávu se znakem `a`.
Odpovědí bude zpráva ve formátu:

    acc;[acc-x];[acc-y];[acc-z]
    
kde význam `[acc-x]`, `[acc-y]` a `[acc-z]` je totožný jako u zprávy [#crash].

Implementace černé skříňky
==========================

Logika černé skříňky byla implementována pomocí jednoduchého stavového automatu,
který je znázorněn na obrázku [#automata].

![Schéma připojení periferních zařízení, 70% #automata](automata.svg)

Ve výchozím stavu `INIT_ACC` se pomocí tzv. alive zprávy ověří funkčnost akcelerometru
a následně dojde k zapnutí funkce snímání zrychlení, která je ve výchozím stavu akcelerometru
po zapnutí vypnutá. Jakmile je akcelerometr inicializován, přejde automat do stavu `WAIT_FOR_CRASH`.

Ve stavu `WAIT_FOR_CRASH` se kit periodicky doptává akcelerometru na aktuální hodnotu
zrychlení ve všech třech osách. V okamžiku, kdy hodnota zrychlení na ose `x` nebo `y`
překročí hodnotu 30000 nebo klesne pod -30000, byl detekován náraz a automat přejde do stavu
`CRASHING`. Hodnota 30000 byla zvolena experimentálně. Nutnost detekovat
prudkou změnu směru ve směru osy `z` se nepředpokládá.

Ve stavu `CRASHING` se kit i nadále periodicky doptává akcelerometru na aktuální zrychlení,
aby se ve zprávě odeslané na server nacházela co nejvyšší hodnota zrychlení, která
nastala, nikoliv naměřená první hodnota, která překročila prahovou úroveň.
V tomto stavu se získané údaje porovnávají s nejvyšší dosud zachycenou hodnotou[^comp]
i s prahovou hodnotou. Při překročení nejvyšší dosud naměřené hodnoty je tato hodnota
aktualizována. V okamžiku klesnutí pod prahovou hodnotu automat přejde
do stavu `SEND_GPS`.

Ve stavu `SEND_GPS` kit načte aktuální polohu z GPS a odešle zprávu s naměřenou hodnotou
zrychlení z minulého kroku a GPS souřadnicí na server. Poté se automat vrátí do stavu
`WAIT_FOR_CRASH`.

Implementace požadavků na aktuální GPS souřadnici nebo zrychlení ze serveru
je řešena pomocí přerušení. V případě žádosti o zrychlení jsou vráceny data,
které byly naposledy získány při čtení dat z akcelerometru ve stavu `WAIT_FOR_CRASH`
nebo `CRASHING`. V případě žádosti o GPS jsou data ze senzoru čtena až v okamžiku,
kdy jsou vyžádána.


[^comp]: Při porovnání je vždy rozhodující vyšší složka z dvojice (abs(x), abs(y))
    a aktualizuje se vždy celá trojice (x, y, z) současně.

Implementace serverové aplikace pro Android
===========================================

...

Autoři
======

Na řešení projektu se podíleli následující autoři:

* Petr Rusiňák (xrusin03) -- programování kitu STM32 Nucleo, dokumentace
* Andrej Hučko (xhucko01) -- vývoj Android aplikace

Závěr
=====

Vytvořené řešení plní roli černé skříňky, která v případě detekce nárazu odešle
informaci místě události a síle nárazu na server...

TODO


