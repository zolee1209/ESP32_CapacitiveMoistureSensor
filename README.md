# Kapacitív talajszonda készítése és illesztése meglévő adatgyűjtő rendszerhez

## Bevezetés: Motiváció - Koncepció

Folyamatban lévő virágnevelési projektemet már kezdetben terveztem automatikus öntözéssel kiegészíteni, amihez szükségem volt a talajnedvesség érzékelésére.

A szenzorral szemben támasztott általános követelmények:
- Prototípusként egyszerű és relatív olcsó megvalósítás
- Tartósság - kültéri üzemeltetés
- Vezetéknélküli működés
- Energiatakarékosság - hosszú üzemidő
- Adatszolgáltatás a meglévő rendszerembe - kihelyezett vezérlési logika

---

## Miért kapacitív mérőszonda?

A szenzorral szemben támasztott elvárásom, miszerint az legyen tartós, az egyszerű rezisztív mérőcsúcsok kiestek. Az olcsó, NYÁK-on létrehozott szenzorfelület galvanikus kapcsolatban van a mért talajjal és az elektrokémiai korrózió viszonylag rövid időn belül tönkreteszi azt. Kapacitív szondáknál a NYÁK felületén lévő forrasztásgátló maszk a kondenzátor fegyverzetét elszigeteli, így az nem korrodálódik.

[https://raspberrypi.stackexchange.com/questions/68133/is-soil-moisture-sensor-corrosion-normal](https://raspberrypi.stackexchange.com/questions/68133/is-soil-moisture-sensor-corrosion-normal)

### Hogyan működik ez a szenzortípus?
A NYÁK-on létrehozott kiterített fegyverzetek és a forrasztásgátló maszk mint dielektrikum, egy kondenzátort alkotnak. Hogy ez a kondenzátor mekkora kapacitással bír, az függ a fegyverzetek méretétől, a közöttük lévő távolságtól és a dielektromos állandótól. Ezt a dielektromos állandót ($` \epsilon_r `$) hangolja el a környezet, jelen esetben a talaj és annak nedvességtartalma. 

$$C = \epsilon_r \epsilon_0 \frac{A}{d}$$

Ennek a kapacitásnak a mérését, vagy annak értékére való következtetést többféleképpen is megtehetjük. Egyik ilyen mérési elv részletesebb leírása a következő linken olvasható: [https://lastminuteengineers.com/capacitive-soil-moisture-sensor-arduino/](https://lastminuteengineers.com/capacitive-soil-moisture-sensor-arduino/) A továbbiakban én is ezzel a metódussal fogok dolgozni.

### Milyen kapacitív szenzorok érhetőek el a piacon?
A piacon többféle NYÁK alapú kapacitív mérőszonda is elérhető az [olcsóbbaktól](https://www.aliexpress.com/item/1005011954309703.html?spm=a2g0o.productlist.main.1.3ff9MMx3MMx33c&algo_pvid=c4e56a0c-48ef-4056-a9f9-d1d863eef6c3&algo_exp_id=c4e56a0c-48ef-4056-a9f9-d1d863eef6c3-9&pdp_ext_f=%7B%22order%22%3A%22106%22%2C%22eval%22%3A%221%22%2C%22fromPage%22%3A%22search%22%7D&pdp_npi=6%40dis%21USD%210.42%210.42%21%21%212.86%212.86%21%402103847817779930340285776e1e0d%2112000057132646679%21sea%21HU%210%21ABX%211%210%21n_tag%3A-29910%3Bd%3A749d5f45%3Bm03_new_user%3A-29895&curPageLogUid=sVzEFhWBFvp7&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005011954309703%7C_p_origin_prod%3A) a [drágábbakig](https://www.aliexpress.com/item/1005008630594130.html?spm=a2g0o.productlist.main.19.3ff9MMx3MMx33c&algo_pvid=6638dd22-3fb5-4115-9675-00f4dfd45f50&algo_exp_id=6638dd22-3fb5-4115-9675-00f4dfd45f50-18&pdp_ext_f=%7B%22order%22%3A%2230%22%2C%22eval%22%3A%221%22%2C%22fromPage%22%3A%22search%22%7D&pdp_npi=6%40dis%21USD%2119.81%2119.81%21%21%2119.81%2119.81%21%402103864c17779929540941092ec96d%2112000046024612017%21sea%21HU%216391153711%21X%211%210%21n_tag%3A-29919%3Bd%3A749d5f45%3Bm03_new_user%3A-29895&curPageLogUid=Y4V8omuxNWiz&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005008630594130%7C_p_origin_prod%3A). Az olcsó alternatíváról a netes fórumokon találhatóak bejegyzések, miszerint nem stabil a kimenetük, eltérő gyártási verziók és minőségek érhetőek el. Kihagyott feszültségstabilizátor, hibás NYÁK terv... [Egy Reddit poszt a témában.](https://www.reddit.com/r/arduino/comments/q1anwt/beware_of_faulty_capacitive_soil_moisture_sensors/) A drágább kategóriából nem akartam vásárolni ehhez a projekthez, mert nem láttam benne az egyszerű integrálhatóságot a saját rendszerembe, bár a nagyobb szenzorfelület rendkívül csábító volt.

Választható alternatíva lett volna még a saját NYÁK tervezése annak minden előnyével és hátrányával, ugyanakkor ezt az opciót időbeli és költségvetési okokból elvetettem.

## Hogyan lehet a szenzor vezeték nélküli, és alacsony fogyasztású?

Maguk a mikrokontrollerek alacsony fogyasztásúnak tekinthetőek, tipikusan párszáz µA/MHz aktív üzemmódban, ezt még befolyásolhatják az egyéb használatban lévő perifériák. Viszont telepes üzemeltetéskor már jelentős fogyasztóvá válhat a mikrovezérlő és minden a telepre kötött pár mA-es áramfelvétellel rendelkező szenzor, modul is.

### Mi a fogyasztás csökkentésének stratégiája?
- A mikrovezérlőn válasszunk optimális órajelet. Ha a feladathoz elégséges és van lehetőség alacsonyabb órajelet választani, tegyük azt.
- A mikrovezérlő aludjon a lehető leghosszabb ideig, a lehető legmélyebben. Általában többféle sleep üzemmód érhető el, melyek során a fogyasztás töredéke az aktív üzemmódhoz viszonyítva, jellemzően a µA-es tartomány alján. Az ébredések sűrűségét a funkció határozza meg. Számomra elegendő - sőt, még feleslegesen sűrű is - percenként mérni egyet.
- Törekedni kell a minél rövidebb aktív időtartamra.
- Amit nem használunk éppen, az ne kapjon tápot. A mérőszenzor táplálása - ha megoldható - történjen az egyik GPIO-ról és kapcsoljuk ki a szenzort amikor nem mérünk vele.

## Milyen eszközökre esett végül a választás?
- **Mikrovezérlő:** [ESP32-C3 Super Mini](https://mischianti.org/esp32-c3-super-mini-high-resolution-pinout-datasheet-and-specs/) Kellően kisméretű fizikailag, igény esetén elég gyors mikrovezérlő, rendelkezik ADC-vel a kapacitív szenzorhoz és WiFi-vel az adatküldéshez. A meglévő adatgyűjtő rendszerem [ESP-NOW](https://www.espressif.com/en/solutions/low-power-solutions/esp-now) protokollt használ, így ez egy sarokpont is volt, hogy az ESP családból válasszak. 
- **Talajszenzor:** Vállalva a ~~rizikót~~ kihívást, az [olcsó](https://www.aliexpress.com/item/1005011954309703.html?spm=a2g0o.productlist.main.1.3ff9MMx3MMx33c&algo_pvid=c4e56a0c-48ef-4056-a9f9-d1d863eef6c3&algo_exp_id=c4e56a0c-48ef-4056-a9f9-d1d863eef6c3-9&pdp_ext_f=%7B%22order%22%3A%22106%22%2C%22eval%22%3A%221%22%2C%22fromPage%22%3A%22search%22%7D&pdp_npi=6%40dis%21USD%210.42%210.42%21%21%212.86%212.86%21%402103847817779930340285776e1e0d%2112000057132646679%21sea%21HU%210%21ABX%211%210%21n_tag%3A-29910%3Bd%3A749d5f45%3Bm03_new_user%3A-29895&curPageLogUid=sVzEFhWBFvp7&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005011954309703%7C_p_origin_prod%3A) szenzor mellett döntöttem. **Jelige:** "Amire nem képes a hardware, azt megoldja a software."
- **Feszültségforrás:** Itt is volt néhány elképzelésem. Kényelmes lett volna kisméretű napelemmel és szuper-kondenzátorral megoldani. Viszont a napelem feszültségkonverziót igényelt volna, a konverzióhoz szükséges alkatrészek miatt kellett volna az egyedi NYÁK. Ezért maradtam az egyszerű Li-ion akkumulátornál és választottam hozzá vezetéknélküli töltőmodult.

![https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/0_electrical_components.jpg](https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/0_electrical_components.jpg)

## Első észrevételek és a nyúl üregének mélysége
- Olyan verziót kaptam a talajszenzorból, ami rendelkezik feszültség- stabilizátorral, nem megfelelő szériás 555-ös IC van rajta és hibás a panelterv, vagyis mindenképp hozzá kell nyúlni.
- Miért gond, hogy rendelkezik a szenzor feszültség- stabilizátorral? Az energiahatékonysági célhoz igazodva, terv szerint az ESP32 GPIO-ja szolgáltatja a szenzor tápfeszültségét. Ez 3,3V lenne. Viszont a feszültség- stabilizátoron mindenképp esik pár tized V, így az [NE555](https://www.ti.com/lit/ds/symlink/ne555.pdf) semmiképp nem kap elegendő feszültséget a stabil működéshez.
- Megfontolandó, hogy a diódás csúcs- egyenirányító pozíciójában alkalmazott ismeretlen típus sem a legmegfelelőbb esetleg. Az itt eső feszültség csökkenti a szenzor mérési tartományát.
- **Konklúzió:** Amennyit szükséges lenne foglalkozni a szenzor NYÁK-kal, hogy tesztelni tudjam "eredeti" formájában, akár le is takaríthatnám róla az összes alkatrészt. Az ESP egyébként is tud négyszögjelet generálni.

#### De akkor hogyan tovább?
- Ha elhagyom a diódát (illetve az integrátor R-C tagot) és nyersen mintavételezem az aluláteresztő- szűrőt, akkor illene tennem egy impedancia- illesztő buffer fokozatot, SW-ben kell megoldanom az integrálást, cserébe növekszik a mérési tartományom, mert elmarad a diódán a feszültségesés. 
- A NYÁK-on lévő érzékelőfelület csekély, kapacitása levegőn 15pF körüli, folyadékba merítve is 300pF környéki értéket vesz fel. Várhatóan talajban a két érték között vesz fel maximum a kontaktfelület tökéletlensége miatt. Az eredeti meghajtó frekvencia az 555-tel 1,5MHz környékén volt, ez az alkalmazott 10k soros ellenállással megfelelő összeállítás.
- Az ESP ADC-je maximum ~100kSPS sebességgel képes mintavételezni, ez viszont rendkívül alacsony ahhoz, hogy meg tudjam mérni az RC-szűrt négyszögjelet. [https://www.allaboutcircuits.com/technical-articles/nyquist-shannon-theorem-understanding-sampled-systems/](https://www.allaboutcircuits.com/technical-articles/nyquist-shannon-theorem-understanding-sampled-systems/)
- **Viszont:** Ha az ESP-vel állítom elő a négyszögjelet, akkor tudom, hogy mikor indult a négyszögjel! És ehhez tudok szinkronizálni. A processzor képes 160MHz-en futni, ami azt jelenti, hogy egészen apró lépésekben a szinkrontól eltolva "végigtapogathatnám" a mérendő jelet más-más periódusba mintavételezve. Mivel a mérendő jel periodikus, az eredeti hullámforma visszaállítható több periódus felhasználásával, így ez egy lehetséges alternatíva. Az alkalmazott metódus neve: [Equivalent Time Sampling - ETS](https://wiki.analog.com/university/tools/m1k/alice/advanced-equivalent-time-sampling-guide)
- Plot twist: Nem működik! :D Az ESP ADC-je nem rendelkezik kellő analóg sávszélességgel, így a négyszögjel rekonstruálása ezen a frekvencián nem volt lehetséges. ~500kHz-ig volt felismerhető, hogy az ESP-vel négyszögjelet generáltam eredetileg. Ennek dokumentációja elmaradt.
- Be kellett látnom, hogy ezekből az eszközökből nem tudok sokkal többet kihozni, mint amire eredeti formájukban képesek voltak. Szükség volt a diódára és az integrátor tagra, cserébe az ESP állította elő a négyszögjelet, így az első deszkamodell ebben a formában készült el.
- Ugyanakkor nem hagyott nyugodni a tudat, hogy a gerjesztő frekvenciát én állítom be... Mi történik, ha nem 1,5MHz környéki frekvenciát használok, hanem valami mást? Kisebbet, vagy nagyobbat. Sokkal kisebbet, vagy sokkal nagyobbat. Nyilván eltérő soros R tagra lenne szükség az eltérő frekvencia miatt, de ez milyen hatással van a talajra, a mérésre? Úgy gondoltam, ha az elektronikában a különböző helyeken az eltérő frekvenciáknak megvannak a sajátos vonzatai, akkor itt is lehet valami extra. De nem tudom, hogy mi. Megkérdeztem ~~mindenttudó~~ Geminit: **"... Tudok-e valamire következtetni, ha eltérő frekvenciákkal mérek?"**
- **A válasz:** "Mit tudsz kideríteni, ha eltérő frekvenciákkal mérsz? (A "Szent Grál")Ha ugyanazt a zónát megméred például $100 \text{ kHz}$-en, majd közvetlenül utána $2 \text{ MHz}$-en, egy fantasztikus dolgot tudsz megbecsülni: a talaj sótartalmát, azaz a tápanyag/műtrágya szintjét (EC - Electrical Conductivity)! Ezt a módszert dielektromos spektroszkópiának hívják.Ennek a jelenségnek az oka a vízben lévő ionok (sók, ásványi anyagok) viselkedése:Alacsony frekvencián (pl. $100 \text{ kHz}$): A PWM jel polaritásváltása elég "lassú" ahhoz, hogy a talajnedvességben lévő ionoknak legyen idejük elmozdulni a pozitív és negatív pólusok felé. Ilyenkor a szenzorod nemcsak a kapacitást, hanem a talaj vezetőképességét (ellenállását) is erősen méri. Ha a talaj sós/tápanyagdús, a mért érték drasztikusan megváltozik a tiszta vízhez képest.Magas frekvencián (pl. $2 \text{ MHz}$ felett): A tér olyan gyorsan váltakozik, hogy a nehéz ionok nem tudják követni a tempót, így mintegy "lemaradnak". Ezen a frekvencián a szenzor szinte kizárólag a víz dielektromos állandójára ($\epsilon_r \approx 80$) reagál, a sótartalom hatása minimális lesz. Tisztán a vízmennyiséget méred."

- Ezután következett újabb trial and error, egy kis kutatómunka, mert nem azt kaptam, amit látni szerettem volna a méréseken. Teszteléshez poharakba készítettem különböző folyadékmintákat: csapvíz, ioncserélt víz, híg- és töményebb tápoldatos vizet. Viszont arányaiban nem láttam eltérést a két mérési frekvenciapár között. Magasabb frekvenciának 1,25MHz, míg alacsonyabbnak 700kHz és 100kHz volt tesztelve. A visszakapott ADC eredmények alapján az látszódott, hogy a szenzor annak megállapítására volt csupán képes, hogy levegőn van-e teljesen, ioncserélt vízben van-e, vagy valamelyik másik tesztmintában. Utóbbiakat egyfromának érzékelte. Sikerült rátalálnom erre a cikkre ami rámutatott, hogy a frekvenciatartományt nem jó irányban tesztelem. [https://metergroup.com/measurement-insights/soil-moisture-sensors-how-they-work-why-some-are-not-research-grade/](https://metergroup.com/measurement-insights/soil-moisture-sensors-how-they-work-why-some-are-not-research-grade/)
Az alacsonyabb frekvenciának meghagytam az 1,25MHz-et a hardware kiépítése miatt, magasabb frekvenciának pedig 40MHz-et választottam, ehhez még volt megfelelő [diódám](https://media.digikey.com/pdf/Data%20Sheets/Infineon%20PDFs/bat62series.pdf) korábbi projektből. Ezáltal már észrevehetővé vált a mérési frekvenciák közötti különbség a különböző tesztminták esetén.

#### Végleges deszkamodellel mért nyers ADC értékek, frekvenciánként 20 egymásutáni mérés átlagai

| tesztminta / frekvencia | 1,25MHz | 40MHz |
|------|----------|-----|
| Levegő | 3786 | 4095 |
| Ioncserélt víz | 2442 | 2645 |
| Csapvíz | 2124 | 2605 |
| Híg tápoldat | 2108 | 2666 |
| Dús tápoldat | 2108 | 2651 |


## Véglegesített kapcsolási rajz

![https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/schematic.JPG](https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/schematic.JPG)


Szerettem volna a tényleges akkumulátor feszültséget mérni, viszont az ehhez szükséges extra alktrészek légszerelésével jelenleg nem akartam időt tölteni. A tápfeszültség ellenőrzése a következőképpen történik: A Li-ion akkumulátor a panel 5V-os tápbemenetére csatlakozik. Ezen van egy [3,3V-os ME6211](https://stm32-base.org/assets/pdf/regulators/ME6211.pdf) feszültségstabilizátor amely rendkívül alacsony, 0,1V-os dropouttal rendelkezik. Ez azt jelenti, hogy ha az akkumulátor feszültsége leesik 3,4V-ra, akkor a feszültségstabilizátor képes még a kimenetén biztosítani a 3,3V-os feszültséget. Ezután ahogy csökken az akku feszültsége, a stabilizátor kimenetén is esni fog a feszültség. Ezt a feszültséget mérjük egy kapcsolható feszültségosztón át, ami a panelre integrált és hozzáadott 10k ellenállásból épül fel. Az eddigi tapasztalatok alapján az így mért feszültségnek kisebb, mint 15LSB változása van. Ez a környező zajokból és az alkatrészek hőmérséklet- függéséből tevődik össze. Várhatóan, ha a stabilizátor kimenete 50mV-ot bezuhan az akkumulátor merülése miatt, a jelenleg mért értékek ~100-zal csökkenni fognak, így jól detektálható a kritikus tápfeszültség megléte.

## Építés

A deszkamodell maradt a végleges forma, az extra alkatrészek a panelre lettek építve.
![https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/1_ESP32-C3_supermini_closeup.jpg](https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/1_ESP32-C3_supermini_closeup.jpg)

Az áramkörnek terveztem, majd 3D nyomtattam egy egymásba csúsztatható burkolatot.
![https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/2_assembly.jpg](https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/2_assembly.jpg)
Ez lehetővé teszi a modulok szükséges mértékű rögzítését és elválasztását.
![https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/3_assembly.jpg](https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/3_assembly.jpg)
Kapott még egy kupakot, így növelve kicsit az elektronika víz elleni védettségét.
![https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/4_final_form.jpg](https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/4_final_form.jpg)

## A software működése

A software működéséről vázlatosan írok csak, a teljes software a [src mappában](https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/tree/main/src) található, Arduino IDE-vel a céleszközre tölthető.

- Az eszköz induláskor / ébredéskor elvégzi a használt ADC-k beállítását, a használt GPIO-kat bemenetre állítja a Built-in LED kivételével, ez output lesz.
- Alacsony szintű kimenetre állítja GPIO1-et ezzel aktiválva a tápfeszültség feszültségosztóját. Vesz 20 ADC mintát, ennek átlagát eltárolja, majd visszaállítja GPIO1-et bemenetté. Ebben a lépésben mérjük meg a tápfeszültséget, mert itt még alacsony az áramkör fogyasztása, az akkumulátor feszültségére való ráhatás itt a legkisebb.
- GPIO6-ot kimenetre állítja és elindítja a 40MHz-es négyszögjel generálását. Vár 100ms-ot, hogy C1 feltöltődjön. Vesz 20 ADC mintát, ennek átlagát eltárolja, majd visszaállítja GPIO6-ot bemenetté. Vár még 500ms-ot, hogy R4 kellően kisüsse C1-et a következő méréshez.
- GPIO7-et kimenetre állítja és elindítja az 1,25MHz-es négyszögjel generálását. Vár 100ms-ot, hogy C1 feltöltődjön. Vesz 20 ADC mintát, ennek átlagát eltárolja, majd visszaállítja GPIO7-et bemenetté. Vár még 500ms-ot, hogy R4 kellően kisüsse C1-et a következő méréshez.
- Alacsony szintre húzza GPIO8-at, ezzel bekapcsolva a panelen lévő kék LED-et.
- Bekapcsolja a WiFi-t, majd ESP-now segítségével elküldi a 3 eltárolt mérési értéket a központi ESP-nek.
- Magas szintre állítja GPIO9-at, ezzel kikapcsolva a kék LED-et.
- Beállítja az alváshoz tartozó időzítőt 60 másodpercre, majd elmegy mélyalvásba. Az időzítő lejárta után az eszköz felébred és az első ponttól folytatja a végrehajtást.

#### A központ...
A központon vevőként funkcionáló ESP sorosporton küldi ki az egységektől kapott adatokat, amiket egy Raspberry Pi 4 dolgoz fel, tárol el és biztosítja a megjelenítést webszerveren keresztül.
![https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/datapoints.png](https://github.com/zolee1209/ESP32_CapacitiveMoistureSensor/blob/main/pictures/datapoints.png)