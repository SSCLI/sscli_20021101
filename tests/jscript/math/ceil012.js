//# ==++== 
//# 
//#   
//#    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//#   
//#    The use and distribution terms for this software are contained in the file
//#    named license.txt, which can be found in the root of this distribution.
//#    By using this software in any fashion, you are agreeing to be bound by the
//#    terms of this license.
//#   
//#    You must not remove this notice, or any other, from this software.
//#   
//# 
//# ==--== 
//####################################################################################
@cc_on


import System;

var NULL_DISPATCH = null;
var apGlobalObj;
var apPlatform;
var lFailCount;


var iTestID = 51770;


function verify( ob, sExp, bugNum) {
    if (bugNum == null) bugNum = "";

    var sAct = Math.ceil(ob);

    if (sAct != sExp)
        apLogFailInfo( m_scen+" failed", sExp, sAct,bugNum);
}

var m_scen = "";

function ceil012() {

    apInitTest("ceil012 ");

    //----------------------------------------------------------------------------
    apInitScenario("1. min pos");

    m_scen = "min pos";

    verify("2.2250738585072014e-308", 1, null);


    //----------------------------------------------------------------------------
    apInitScenario("2. min pos < n < max pos, up to full display precision");
    
    m_scen = "min pos < n < max pos, up to full display precision";

    verify(".6", 1, null);
    verify("2.", 2, null);
    verify(".44", 1, null);
    verify("7.2", 8, null);
    verify("14.", 14, null);
    verify(".786", 1, null);
    verify("7.03", 8, null);
    verify("86.6", 87, null);
    verify("135.", 135, null);
    verify(".1198", 1, null);
    verify("0.287", 1, null);
    verify("17.55", 18, null);
    verify("540.0", 540, null);
    verify("6076.", 6076, null);
    verify(".62153", 1, null);
    verify("2.6611", 3, null);
    verify("96.121", 97, null);
    verify("489.20", 490, null);
    verify("5499.2", 5500, null);
    verify("90415.", 90415, null);
    verify(".155358", 1, null);
    verify("2.02021", 3, null);
    verify("94.1046", 95, null);
    verify("564.804", 565, null);
    verify("7918.40", 7919, null);
    verify("68048.8", 68049, null);
    verify("213309.", 213309, null);
    verify(".1226468", 1, null);
    verify("9.544780", 10, null);
    verify("88.08627", 89, null);
    verify("772.8569", 773, null);
    verify("7405.900", 7406, null);
    verify("52981.86", 52982, null);
    verify("557085.8", 557086, null);
    verify("2446900.", 2446900, null);
    verify(".72709566", 1, null);
    verify("5.2120043", 6, null);
    verify("13.252459", 14, null);
    verify("241.43360", 242, null);
    verify("4458.9784", 4459, null);
    verify("80312.218", 80313, null);
    verify("696010.94", 696011, null);
    verify("9958380.8", 9958381, null);
    verify("78504937.", 78504937, null);
    verify(".143470961", 1, null);
    verify("1.23601932", 2, null);
    verify("39.0912734", 40, null);
    verify("463.555346", 464, null);
    verify("1312.74929", 1313, null);
    verify("98224.9972", 98225, null);
    verify("595098.763", 595099, null);
    verify("7061195.21", 7061196, null);
    verify("75903211.0", 75903211, null);
    verify("690839080.", 690839080, null);
    verify(".8807708514", 1, null);
    verify("4.088911435", 5, null);
    verify("75.27016363", 76, null);
    verify("561.0251120", 562, null);
    verify("5562.421819", 5563, null);
    verify("48013.26405", 48014, null);
    verify("318622.0868", 318623, null);
    verify("2255066.563", 2255067, null);
    verify("63141035.99", 63141036, null);
    verify("152643838.4", 152643839, null);
    verify("4132283254.", 4132283254, null);
    verify(".90337464930", 1, null);
    verify("2.5474295081", 3, null);
    verify("56.356969717", 57, null);
    verify("197.23794979", 198, null);
    verify("7226.6664991", 7227, null);
    verify("47583.278459", 47584, null);
    verify("155763.59386", 155764, null);
    verify("6237632.6977", 6237633, null);
    verify("45252125.874", 45252126, null);
    verify("475920249.94", 475920250, null);
    verify("5531669963.8", 5531669964, null);
    verify("71875996973.", 71875996973, null);
    verify("72860084283", 72860084283, null);
    verify(".148158349250", 1, null);
    verify("5.10823278330", 6, null);
    verify("86.1765834983", 87, null);
    verify("410.944670926", 411, null);
    verify("3101.12061200", 3102, null);
    verify("52944.3618810", 52945, null);
    verify("207709.254881", 207710, null);
    verify("4977900.91535", 4977901, null);
    verify("79846204.7014", 79846205, null);
    verify("792294719.622", 792294720, null);
    verify("1030065957.74", 1030065958, null);
    verify("22745604335.8", 22745604336, null);
    verify("978322384890.", 978322384890, null);
    verify("943471893780", 943471893780, null);
    verify(".2071476641772", 1, null);
    verify("5.197179852767", 6, null);
    verify("28.92491713587", 29, null);
    verify("213.3537603013", 214, null);
    verify("7105.498363543", 7106, null);
    verify("53209.09272198", 53210, null);
    verify("365883.2780820", 365884, null);
    verify("1824205.070693", 1824206, null);
    verify("83667863.76719", 83667864, null);
    verify("587065498.1415", 587065499, null);
    verify("1234297163.026", 1234297164, null);
    verify("69903937280.88", 69903937281, null);
    verify("507570635327.0", 507570635327, null);
    verify("7159818530489.", 7159818530489, null);
    verify("9415642957298", 9415642957298, null);
    verify(".68920778576504", 1, null);
    verify("2.5348921384159", 3, null);
    verify("78.064458756760", 79, null);
    verify("413.92148252314", 414, null);
    verify("3291.2624079456", 3292, null);
    verify("72462.406537462", 72463, null);
    verify("660928.11304554", 660929, null);
    verify("9090729.6320133", 9090730, null);
    verify("96018819.280453", 96018820, null);
    verify("496567687.42381", 496567688, null);
    verify("6216971651.0665", 6216971652, null);
    verify("38220781319.655", 38220781320, null);
    verify("253064691100.61", 253064691101, null);
    verify("9068799484665.9", 9068799484666, null);
    verify("35400069447487.", 35400069447487, null);
    verify("43823967881364", 43823967881364, null);
    verify(".727869124134605", 1, null);
    verify("7.89049944596633", 8, null);
    verify("38.8539718544432", 39, null);
    verify("386.440145639707", 387, null);
    verify("9941.26468948541", 9942, null);
    verify("15637.3266253974", 15638, null);
    verify("553929.976613362", 553930, null);
    verify("6322851.30065633", 6322852, null);
    verify("63872380.3346230", 63872381, null);
    verify("376480639.494894", 376480640, null);
    verify("1209785286.94289", 1209785287, null);
    verify("27835156131.2802", 27835156132, null);
    verify("981668226611.797", 981668226612, null);
    verify("6716339086274.29", 6716339086275, null);
    verify("31018751905191.5", 31018751905192, null);
    verify("690931313397259.", 690931313397259, null);
    verify("225264066871575", 225264066871575, null);


    //----------------------------------------------------------------------------
    apInitScenario("3. min pos < n < 1, e notation");

    m_scen = "min pos < n < 1, e notation";

    verify("3.2e-0", 4, null);
    verify("3.2e-1", 1, null);
    verify("3.2e-2", 1, null);
    verify("4.85e-1", 1, null);
    verify("4.85e-2", 1, null);
    verify("4.85e-3", 1, null);
    verify("8.498e-2", 1, null);
    verify("8.498e-3", 1, null);
    verify("8.498e-4", 1, null);
    verify("2.3424e-3", 1, null);
    verify("2.3424e-4", 1, null);
    verify("2.3424e-5", 1, null);
    verify("6.44322e-4", 1, null);
    verify("6.44322e-5", 1, null);
    verify("6.44322e-6", 1, null);
    verify("3.162709e-5", 1, null);
    verify("3.162709e-6", 1, null);
    verify("3.162709e-7", 1, null);
    verify("8.3030443e-6", 1, null);
    verify("8.3030443e-7", 1, null);
    verify("8.3030443e-8", 1, null);
    verify("2.86347466e-7", 1, null);
    verify("2.86347466e-8", 1, null);
    verify("2.86347466e-9", 1, null);
    verify("4.251479468e-8", 1, null);
    verify("4.251479468e-9", 1, null);
    verify("4.251479468e-10", 1, null);
    verify("1.7389808272e-9", 1, null);
    verify("1.7389808272e-10", 1, null);
    verify("1.7389808272e-11", 1, null);
    verify("5.95856989484e-10", 1, null);
    verify("5.95856989484e-11", 1, null);
    verify("5.95856989484e-12", 1, null);
    verify("5.312540051362e-11", 1, null);
    verify("5.312540051362e-12", 1, null);
    verify("5.312540051362e-13", 1, null);
    verify("3.7016780183062e-12", 1, null);
    verify("3.7016780183062e-13", 1, null);
    verify("3.7016780183062e-14", 1, null);
    verify("8.24431554355400e-13", 1, null);
    verify("8.24431554355400e-14", 1, null);
    verify("8.24431554355400e-15", 1, null);
    verify("6.24673773175102e-14", 1, null);
    verify("6.24673773175102e-15", 1, null);
    verify("6.24673773175102e-16", 1, null);
    verify("3.20960934796270e-15", 1, null);
    verify("3.20960934796270e-16", 1, null);
    verify("3.20960934796270e-17", 1, null);
    verify("6.70088883684615e-16", 1, null);
    verify("6.70088883684615e-17", 1, null);
    verify("6.70088883684615e-18", 1, null);
    verify("7.76359016862755e-17", 1, null);
    verify("7.76359016862755e-18", 1, null);
    verify("7.76359016862755e-19", 1, null);
    verify("5.88691141009287e-18", 1, null);
    verify("5.88691141009287e-19", 1, null);
    verify("5.88691141009287e-20", 1, null);
    verify("3.09492250471093e-19", 1, null);
    verify("3.09492250471093e-20", 1, null);
    verify("3.09492250471093e-21", 1, null);
    verify("8.19457916709552e-20", 1, null);
    verify("8.19457916709552e-21", 1, null);
    verify("8.19457916709552e-22", 1, null);
    verify("8.14085943218543e-21", 1, null);
    verify("8.14085943218543e-22", 1, null);
    verify("8.14085943218543e-23", 1, null);
    verify("9.76007205420988e-22", 1, null);
    verify("9.76007205420988e-23", 1, null);
    verify("9.76007205420988e-24", 1, null);
    verify("3.82611861733991e-105", 1, null);
    verify("3.82611861733991e-106", 1, null);
    verify("3.82611861733991e-107", 1, null);
    verify("5.43393900702960e-305", 1, null);
    verify("5.43393900702960e-306", 1, null);
    verify("5.43393900702960e-307", 1, null);
    verify("7.40832645012184e-306", 1, null);
    verify("7.40832645012184e-307", 1, null);
    verify("7.40832645012184e-308", 1, null);


    //----------------------------------------------------------------------------
    apInitScenario("4. min pos < n < max pos, e notation");

    m_scen = "min pos < n < max pos, e notation";

    verify("4.4e1", 44, null);
    verify("4.4e2", 440, null);
    verify("4.4e3", 4400, null);
    verify("5.28e2", 528, null);
    verify("5.28e3", 5280, null);
    verify("5.28e4", 52800, null);
    verify("9.762e3", 9762, null);
    verify("9.762e4", 97620, null);
    verify("9.762e5", 976200, null);
    verify("9.5400e4", 95400, null);
    verify("9.5400e5", 954000, null);
    verify("9.5400e6", 9540000, null);
    verify("9.31816e5", 931816, null);
    verify("9.31816e6", 9318160, null);
    verify("9.31816e7", 93181600, null);
    verify("2.269759e6", 2269759, null);
    verify("2.269759e7", 22697590, null);
    verify("2.269759e8", 226975900, null);
    verify("5.1924641e7", 51924641, null);
    verify("5.1924641e8", 519246410, null);
    verify("5.1924641e9", 5192464100, null);
    verify("6.97184331e8", 697184331, null);
    verify("6.97184331e9", 6971843310, null);
    verify("6.97184331e10", 69718433100, null);
    verify("3.083970371e9", 3083970371, null);
    verify("3.083970371e10", 30839703710, null);
    verify("3.083970371e11", 308397037100, null);
    verify("9.0006131232e10", 90006131232, null);
    verify("9.0006131232e11", 900061312320, null);
    verify("9.0006131232e12", 9000613123200, null);
    verify("5.52474526151e11", 552474526151, null);
    verify("5.52474526151e12", 5524745261510, null);
    verify("5.52474526151e13", 55247452615100, null);
    verify("2.809281384713e12", 2809281384713, null);
    verify("2.809281384713e13", 28092813847130, null);
    verify("2.809281384713e14", 280928138471300, null);
    verify("7.4168953575150e13", 74168953575150, null);
    verify("7.4168953575150e14", 741689535751500, null);
    verify("7.4168953575150e15", 7416895357515000, null);
    verify("3.29591278725293e14", 329591278725293, null);
    verify("3.29591278725293e15", 3295912787252930, null);
    verify("3.29591278725293e16", 32959127872529300, null);
    verify("7.00871941301623e15", 7008719413016230, null);
    verify("7.00871941301623e16", 70087194130162300, null);
    verify("7.00871941301623e17", 700871941301623040, null);
    verify("1.44829343149756e16", 14482934314975600, null);
    verify("1.44829343149756e17", 144829343149756000, null);
    verify("1.44829343149756e18", 1448293431497560000, null);
    verify("4.32827565004832e17", 432827565004832000, null);
    verify("4.32827565004832e18", 4328275650048320000, null);
    verify("4.32827565004832e19", 43282756500483203000, null);
    verify("1.76150125345059e18", 1761501253450590000, null);
    verify("1.76150125345059e19", 17615012534505900000, null);
    verify("1.76150125345059e20", 1.7615012534505901e+020, null);
    verify("4.61563134677205e19", 46156313467720499000, null);
    verify("4.61563134677205e20", 4.6156313467720499e+020, null);
    verify("4.61563134677205e21", 4.6156313467720502e+021, null);
    verify("1.91203163171897e20", 1.9120316317189702e+020, null);
    verify("1.91203163171897e21", 1.91203163171897e+021, null);
    verify("1.91203163171897e22", 1.9120316317189701e+022, null);
    verify("4.03602472647321e21", 4.0360247264732098e+021, null);
    verify("4.03602472647321e22", 4.0360247264732101e+022, null);
    verify("4.03602472647321e23", 4.0360247264732101e+023, null);
    verify("7.49015326317356e22", 7.4901532631735596e+022, null);
    verify("7.49015326317356e23", 7.4901532631735598e+023, null);
    verify("7.49015326317356e24", 7.4901532631735603e+024, null);
    verify("7.69755712492805e23", 7.6975571249280504e+023, null);
    verify("7.69755712492805e24", 7.6975571249280499e+024, null);
    verify("7.69755712492805e25", 7.6975571249280499e+025, null);
    verify("1.25663813681152e106", 1.2566381368115199e+106, null);
    verify("1.25663813681152e107", 1.25663813681152e+107, null);
    verify("1.25663813681152e108", 1.2566381368115199e+108, null);
    verify("9.49682486188604e305", 9.4968248618860401e+305, null);
    verify("9.49682486188604e306", 9.4968248618860404e+306, null);
    verify("9.49682486188604e307", 9.4968248618860399e+307, null);
    verify("4.74963635744467e306", 4.7496363574446698e+306, null);
    verify("4.74963635744467e307", 4.7496363574446703e+307, null);
    verify("4.74963635744467e308", 4.74963635744467e+308, null);
    verify("6.73492661754150e307", 6.7349266175414999e+307, null);
    verify("6.73492661754150e308", 6.73492661754150e+308, null);
    verify("6.73492661754150e309", 6.73492661754150e+309, null);

    //----------------------------------------------------------------------------
    apInitScenario("4. max pos");

    m_scen = "max pos";

    verify("1.7976931348623158e308",1.7976931348623158e308, null);

    
    //----------------------------------------------------------------------------
    apInitScenario("5. > max pos float (1.#INF)");
    
    m_scen = "> max pos float (1.#INF)";

    verify("1.797693134862315807e309",1.797693134862315807e309, null);

    
    //----------------------------------------------------------------------------
    apInitScenario("6. max neg");

    m_scen = "max neg";

    verify("-2.2250738585072012595e-308",0, null);


    //----------------------------------------------------------------------------
    apInitScenario("7. max neg > n > min neg, up to full display precision");
    
    m_scen = "max neg > n > min neg, up to full display precision";

    verify("-.5", 0, null);
    verify("-5.", -5, null);
    verify("-.96", 0, null);
    verify("-3.1", -3, null);
    verify("-46.", -46, null);
    verify("-.479", 0, null);
    verify("-1.49", -1, null);
    verify("-95.5", -95, null);
    verify("-571.", -571, null);
    verify("-.4592", 0, null);
    verify("-0.569", 0, null);
    verify("-26.33", -26, null);
    verify("-543.6", -543, null);
    verify("-5967.", -5967, null);
    verify("-.90277", 0, null);
    verify("-7.9163", -7, null);
    verify("-41.279", -41, null);
    verify("-435.14", -435, null);
    verify("-8087.2", -8087, null);
    verify("-93731.", -93731, null);
    verify("-.905499", 0, null);
    verify("-2.40249", -2, null);
    verify("-49.7373", -49, null);
    verify("-616.218", -616, null);
    verify("-3380.45", -3380, null);
    verify("-38655.0", -38655, null);
    verify("-330284.", -330284, null);
    verify("-.3311229", 0, null);
    verify("-2.258134", -2, null);
    verify("-57.53541", -57, null);
    verify("-296.2066", -296, null);
    verify("-5262.788", -5262, null);
    verify("-69894.69", -69894, null);
    verify("-817561.2", -817561, null);
    verify("-1508910.", -1508910, null);
    verify("-.30719336", 0, null);
    verify("-3.6205174", -3, null);
    verify("-49.521465", -49, null);
    verify("-293.83437", -293, null);
    verify("-9940.2411", -9940, null);
    verify("-40721.260", -40721, null);
    verify("-588214.95", -588214, null);
    verify("-4061036.7", -4061036, null);
    verify("-14240519.", -14240519, null);
    verify("-.313171193", 0, null);
    verify("-4.31762894", -4, null);
    verify("-51.2635061", -51, null);
    verify("-678.616716", -678, null);
    verify("-3690.35822", -3690, null);
    verify("-77552.1661", -77552, null);
    verify("-307853.281", -307853, null);
    verify("-8827044.56", -8827044, null);
    verify("-65487957.7", -65487957, null);
    verify("-368544489.", -368544489, null);
    verify("-.1991785416", 0, null);
    verify("-2.272555041", -2, null);
    verify("-78.77000769", -78, null);
    verify("-848.5457325", -848, null);
    verify("-2501.275743", -2501, null);
    verify("-20022.56009", -20022, null);
    verify("-772824.9969", -772824, null);
    verify("-8111880.441", -8111880, null);
    verify("-32635057.88", -32635057, null);
    verify("-959661249.6", -959661249, null);
    verify("-3981052388.", -3981052388, null);
    verify("-9075243880", -9075243880, null);
    verify("-.60844235274", 0, null);
    verify("-3.0151020907", -3, null);
    verify("-91.410728930", -91, null);
    verify("-655.18392972", -655, null);
    verify("-6895.7597385", -6895, null);
    verify("-36894.158554", -36894, null);
    verify("-380261.89037", -380261, null);
    verify("-4585902.3424", -4585902, null);
    verify("-58454441.454", -58454441, null);
    verify("-949203226.66", -949203226, null);
    verify("-1401348344.2", -1401348344, null);
    verify("-76363606118.", -76363606118, null);
    verify("-95660430789", -95660430789, null);
    verify("-.586931356752", 0, null);
    verify("-1.62114054111", -1, null);
    verify("-31.0670443374", -31, null);
    verify("-772.618954449", -772, null);
    verify("-9226.13843442", -9226, null);
    verify("-32393.5294507", -32393, null);
    verify("-567241.586742", -567241, null);
    verify("-5885209.59360", -5885209, null);
    verify("-16405512.2467", -16405512, null);
    verify("-332285057.260", -332285057, null);
    verify("-7311868918.51", -7311868918, null);
    verify("-22516507490.4", -22516507490, null);
    verify("-512925511957.", -512925511957, null);
    verify("-604644063961", -604644063961, null);
    verify("-.3489196701652", 0, null);
    verify("-8.764807916087", -8, null);
    verify("-28.25601763587", -28, null);
    verify("-514.7378519873", -514, null);
    verify("-3593.085212360", -3593, null);
    verify("-40542.41949671", -40542, null);
    verify("-370979.9626897", -370979, null);
    verify("-7893828.344884", -7893828, null);
    verify("-18948367.87536", -18948367, null);
    verify("-645330477.8117", -645330477, null);
    verify("-6263973064.869", -6263973064, null);
    verify("-92229379100.37", -92229379100, null);
    verify("-283988953726.7", -283988953726, null);
    verify("-8883069582080.", -8883069582080, null);
    verify("-8969247128178", -8969247128178, null);
    verify("-.99747464946105", 0, null);
    verify("-7.8390010385020", -7, null);
    verify("-38.706642505934", -38, null);
    verify("-533.18973357646", -533, null);
    verify("-1197.1252686590", -1197, null);
    verify("-46082.538756131", -46082, null);
    verify("-799827.87110566", -799827, null);
    verify("-4709736.1173721", -4709736, null);
    verify("-77450602.685780", -77450602, null);
    verify("-550693548.34689", -550693548, null);
    verify("-6661523519.2974", -6661523519, null);
    verify("-58640312226.305", -58640312226, null);
    verify("-560690445612.22", -560690445612, null);
    verify("-1396666139436.1", -1396666139436, null);
    verify("-13595809502008.", -13595809502008, null);
    verify("-93482603370539", -93482603370539, null);
    verify("-.946037233674496", 0, null);
    verify("-5.48428455805845", -5, null);
    verify("-94.1054707644185", -94, null);
    verify("-113.409414264112", -113, null);
    verify("-2814.49360265583", -2814, null);
    verify("-90108.1357162047", -90108, null);
    verify("-863454.843502128", -863454, null);
    verify("-9795197.06271489", -9795197, null);
    verify("-13044202.5876058", -13044202, null);
    verify("-381640833.081739", -381640833, null);
    verify("-9870443883.43428", -9870443883, null);
    verify("-60422412387.5419", -60422412387, null);
    verify("-628182548242.912", -628182548242, null);
    verify("-8149733900124.40", -8149733900124, null);
    verify("-92432576712182.4", -92432576712182, null);
    verify("-710446389100490.", -710446389100490, null);
    verify("-990962943490046", -990962943490046, null);


    //----------------------------------------------------------------------------
    apInitScenario("8. max neg > n > -1, e notation");

    m_scen = "max neg > n > -1, e notation";

    verify("-5.0e-0", -5, null);
    verify("-5.0e-1", 0, null);
    verify("-5.0e-2", 0, null);
    verify("-1.44e-1", 0, null);
    verify("-1.44e-2", 0, null);
    verify("-1.44e-3", 0, null);
    verify("-1.927e-2", 0, null);
    verify("-1.927e-3", 0, null);
    verify("-1.927e-4", 0, null);
    verify("-5.4511e-3", 0, null);
    verify("-5.4511e-4", 0, null);
    verify("-5.4511e-5", 0, null);
    verify("-9.00017e-4", 0, null);
    verify("-9.00017e-5", 0, null);
    verify("-9.00017e-6", 0, null);
    verify("-4.837171e-5", 0, null);
    verify("-4.837171e-6", 0, null);
    verify("-4.837171e-7", 0, null);
    verify("-6.8920901e-6", 0, null);
    verify("-6.8920901e-7", 0, null);
    verify("-6.8920901e-8", 0, null);
    verify("-7.75575499e-7", 0, null);
    verify("-7.75575499e-8", 0, null);
    verify("-7.75575499e-9", 0, null);
    verify("-3.784053351e-8", 0, null);
    verify("-3.784053351e-9", 0, null);
    verify("-3.784053351e-10", 0, null);
    verify("-4.2510597333e-9", 0, null);
    verify("-4.2510597333e-10", 0, null);
    verify("-4.2510597333e-11", 0, null);
    verify("-1.27228923449e-10", 0, null);
    verify("-1.27228923449e-11", 0, null);
    verify("-1.27228923449e-12", 0, null);
    verify("-2.803018452148e-11", 0, null);
    verify("-2.803018452148e-12", 0, null);
    verify("-2.803018452148e-13", 0, null);
    verify("-2.7110671628691e-12", 0, null);
    verify("-2.7110671628691e-13", 0, null);
    verify("-2.7110671628691e-14", 0, null);
    verify("-9.56217783273567e-13", 0, null);
    verify("-9.56217783273567e-14", 0, null);
    verify("-9.56217783273567e-15", 0, null);
    verify("-6.99456620166961e-14", 0, null);
    verify("-6.99456620166961e-15", 0, null);
    verify("-6.99456620166961e-16", 0, null);
    verify("-3.40005567324267e-15", 0, null);
    verify("-3.40005567324267e-16", 0, null);
    verify("-3.40005567324267e-17", 0, null);
    verify("-6.17227311203627e-16", 0, null);
    verify("-6.17227311203627e-17", 0, null);
    verify("-6.17227311203627e-18", 0, null);
    verify("-9.36645803333569e-17", 0, null);
    verify("-9.36645803333569e-18", 0, null);
    verify("-9.36645803333569e-19", 0, null);
    verify("-7.26859340546239e-18", 0, null);
    verify("-7.26859340546239e-19", 0, null);
    verify("-7.26859340546239e-20", 0, null);
    verify("-7.62866563135477e-19", 0, null);
    verify("-7.62866563135477e-20", 0, null);
    verify("-7.62866563135477e-21", 0, null);
    verify("-2.43012881718031e-20", 0, null);
    verify("-2.43012881718031e-21", 0, null);
    verify("-2.43012881718031e-22", 0, null);
    verify("-7.97350417926250e-21", 0, null);
    verify("-7.97350417926250e-22", 0, null);
    verify("-7.97350417926250e-23", 0, null);
    verify("-2.37745678018969e-22", 0, null);
    verify("-2.37745678018969e-23", 0, null);
    verify("-2.37745678018969e-24", 0, null);
    verify("-7.27813770416071e-105", 0, null);
    verify("-7.27813770416071e-106", 0, null);
    verify("-7.27813770416071e-107", 0, null);
    verify("-1.30581979540712e-305", 0, null);
    verify("-1.30581979540712e-306", 0, null);
    verify("-1.30581979540712e-307", 0, null);
    verify("-2.64396529892862e-306", 0, null);
    verify("-2.64396529892862e-307", 0, null);
    verify("-2.64396529892862e-308", 0, null);


    //----------------------------------------------------------------------------
    apInitScenario("9. max neg > n > min neg, e notation");

    m_scen = "max neg > n > min neg, e notation";

    verify("-8.5e0", -8, null);
    verify("-8.5e1", -85, null);
    verify("-8.5e2", -850, null);
    verify("-1.52e1", -15, null);
    verify("-1.52e2", -152, null);
    verify("-1.52e3", -1520, null);
    verify("-5.363e2", -536, null);
    verify("-5.363e3", -5363, null);
    verify("-5.363e4", -53630, null);
    verify("-2.0302e3", -2030, null);
    verify("-2.0302e4", -20302, null);
    verify("-2.0302e5", -203020, null);
    verify("-4.06856e4", -40685, null);
    verify("-4.06856e5", -406856, null);
    verify("-4.06856e6", -4068560, null);
    verify("-3.526625e5", -352662, null);
    verify("-3.526625e6", -3526625, null);
    verify("-3.526625e7", -35266250, null);
    verify("-4.2131970e6", -4213197, null);
    verify("-4.2131970e7", -42131970, null);
    verify("-4.2131970e8", -421319700, null);
    verify("-9.39827625e7", -93982762, null);
    verify("-9.39827625e8", -939827625, null);
    verify("-9.39827625e9", -9398276250, null);
    verify("-5.296667600e8", -529666760, null);
    verify("-5.296667600e9", -5296667600, null);
    verify("-5.296667600e10", -52966676000, null);
    verify("-2.9894724429e9", -2989472442, null);
    verify("-2.9894724429e10", -29894724429, null);
    verify("-2.9894724429e11", -298947244290, null);
    verify("-9.31269515836e10", -93126951583, null);
    verify("-9.31269515836e11", -931269515836, null);
    verify("-9.31269515836e12", -9312695158360, null);
    verify("-9.785227422363e11", -978522742236, null);
    verify("-9.785227422363e12", -9785227422363, null);
    verify("-9.785227422363e13", -97852274223630, null);
    verify("-6.2804629881637e12", -6280462988163, null);
    verify("-6.2804629881637e13", -62804629881637, null);
    verify("-6.2804629881637e14", -628046298816370, null);
    verify("-1.97084335264933e13", -19708433526493, null);
    verify("-1.97084335264933e14", -197084335264933, null);
    verify("-1.97084335264933e15", -1970843352649330, null);
    verify("-9.87601801405715e14", -987601801405715, null);
    verify("-9.87601801405715e15", -9876018014057150, null);
    verify("-9.87601801405715e16", -98760180140571500, null);
    verify("-2.00941085362823e15", -2009410853628230, null);
    verify("-2.00941085362823e16", -20094108536282300, null);
    verify("-2.00941085362823e17", -200941085362823000, null);
    verify("-6.75649868607965e16", -67564986860796496, null);
    verify("-6.75649868607965e17", -675649868607965060, null);
    verify("-6.75649868607965e18", -6756498686079649800, null);
    verify("-7.43963320455925e17", -743963320455924990, null);
    verify("-7.43963320455925e18", -7439633204559250400, null);
    verify("-7.43963320455925e19", -74396332045592494000, null);
    verify("-5.37604621849243e18", -5376046218492430300, null);
    verify("-5.37604621849243e19", -53760462184924299000, null);
    verify("-5.37604621849243e20", -5.3760462184924303e+020, null);
    verify("-1.36075801208007e19", -13607580120800700000, null);
    verify("-1.36075801208007e20", -1.36075801208007e+020, null);
    verify("-1.36075801208007e21", -1.3607580120800701e+021, null);
    verify("-9.30987673607026e20", -9.3098767360702598e+020, null);
    verify("-9.30987673607026e21", -9.3098767360702598e+021, null);
    verify("-9.30987673607026e22", -9.3098767360702592e+022, null);
    verify("-5.75728671948621e21", -5.7572867194862101e+021, null);
    verify("-5.75728671948621e22", -5.7572867194862099e+022, null);
    verify("-5.75728671948621e23", -5.75728671948621e+023, null);
    verify("-5.59643799688426e22", -5.5964379968842603e+022, null);
    verify("-5.59643799688426e23", -5.5964379968842603e+023, null);
    verify("-5.59643799688426e24", -5.5964379968842596e+024, null);
    verify("-3.14403816749344e105", -3.1440381674934399e+105, null);
    verify("-3.14403816749344e106", -3.1440381674934401e+106, null);
    verify("-3.14403816749344e107", -3.1440381674934399e+107, null);
    verify("-8.30817936376931e305", -8.3081793637693096e+305, null);
    verify("-8.30817936376931e306", -8.3081793637693106e+306, null);
    verify("-8.30817936376931e307", -8.3081793637693096e+307, null);
    verify("-8.20646232996096e306", -8.2064623299609596e+306, null);
    verify("-8.20646232996096e307", -8.2064623299609596e+307, null);
    verify("-8.20646232996096e308", -8.20646232996096e+309, null);
    
    //----------------------------------------------------------------------------
    apInitScenario("9. min neg");
    
    m_scen = "min neg";

    verify("-1.797693134862315807e308", -1.797693134862315807e308, null);


    //----------------------------------------------------------------------------
    apInitScenario("10. < min neg float (-1.#INF)");
    
    m_scen = "< min neg float (-1.#INF)";

    verify("-1.797693134862315807e309",-1.797693134862315807e309, null);


    //----------------------------------------------------------------------------
    apInitScenario("11. zero");
    
    m_scen = "zero";
    
    verify("0.0",0, null);


    apEndTest();

}



ceil012();


if(lFailCount >= 0) System.Environment.ExitCode = lFailCount;
else System.Environment.ExitCode = 1;

function apInitTest(stTestName) {
    lFailCount = 0;

    apGlobalObj = new Object();
    apGlobalObj.apGetPlatform = function Funca() { return "Rotor" }
    apGlobalObj.LangHost = function Funcb() { return 1033;}
    apGlobalObj.apGetLangExt = function Funcc(num) { return "EN"; }

    apPlatform = apGlobalObj.apGetPlatform();
    var sVer = "1.0";  //navigator.appVersion.toUpperCase().charAt(navigator.appVersion.toUpperCase().indexOf("MSIE")+5);
    apGlobalObj.apGetHost = function Funcp() { return "Rotor " + sVer; }
    print ("apInitTest: " + stTestName);
}

function apInitScenario(stScenarioName) {print( "\tapInitScenario: " + stScenarioName);}

function apLogFailInfo(stMessage, stExpected, stActual, stBugNum) {
    lFailCount = lFailCount + 1;
    print ("***** FAILED:");
    print ("\t\t" + stMessage);
    print ("\t\tExpected: " + stExpected);
    print ("\t\tActual: " + stActual);
}

function apGetLocale(){ return 1033; }
function apWriteDebug(s) { print("dbg ---> " + s) }
function apEndTest() {}
