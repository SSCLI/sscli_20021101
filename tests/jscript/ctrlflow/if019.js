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


var iTestID = 52841;


function if019() 
{
    apInitTest("if019 ");

    //----------------------------------------------------------------------------
    apInitScenario("1. verify deep nesting");

// Added to handle strict mode in JScript 7.0
@cc_on 
    @if (@_fast)  
        var n;        
     
@end       

    if (false) n=0;
	else if (false) n=1;
	else if (false) n=2;
	else if (false) n=3;
	else if (false) n=4;
	else if (false) n=5;
	else if (false) n=6;
	else if (false) n=7;
	else if (false) n=8;
	else if (false) n=9;
	else if (false) n=10;
	else if (false) n=11;
	else if (false) n=12;
	else if (false) n=13;
	else if (false) n=14;
	else if (false) n=15;
	else if (false) n=16;
	else if (false) n=17;
	else if (false) n=18;
	else if (false) n=19;
	else if (false) n=20;
	else if (false) n=21;
	else if (false) n=22;
	else if (false) n=23;
	else if (false) n=24;
	else if (false) n=25;
	else if (false) n=26;
	else if (false) n=27;
	else if (false) n=28;
	else if (false) n=29;
	else if (false) n=30;
	else if (false) n=31;
	else if (false) n=32;
	else if (false) n=33;
	else if (false) n=34;
	else if (false) n=35;
	else if (false) n=36;
	else if (false) n=37;
	else if (false) n=38;
	else if (false) n=39;
	else if (false) n=40;
	else if (false) n=41;
	else if (false) n=42;
	else if (false) n=43;
	else if (false) n=44;
	else if (false) n=45;
	else if (false) n=46;
	else if (false) n=47;
	else if (false) n=48;
	else if (false) n=49;
	else if (false) n=50;
	else if (false) n=51;
	else if (false) n=52;
	else if (false) n=53;
	else if (false) n=54;
	else if (false) n=55;
	else if (false) n=56;
	else if (false) n=57;
	else if (false) n=58;
	else if (false) n=59;
	else if (false) n=60;
	else if (false) n=61;
	else if (false) n=62;
	else if (false) n=63;
	else if (false) n=64;
	else if (false) n=65;
	else if (false) n=66;
	else if (false) n=67;
	else if (false) n=68;
	else if (false) n=69;
	else if (false) n=70;
	else if (false) n=71;
	else if (false) n=72;
	else if (false) n=73;
	else if (false) n=74;
	else if (false) n=75;
	else if (false) n=76;
	else if (false) n=77;
	else if (false) n=78;
	else if (false) n=79;
	else if (false) n=80;
	else if (false) n=81;
	else if (false) n=82;
	else if (false) n=83;
	else if (false) n=84;
	else if (false) n=85;
	else if (false) n=86;
	else if (false) n=87;
	else if (false) n=88;
	else if (false) n=89;
	else if (false) n=90;
	else if (false) n=91;
	else if (false) n=92;
	else if (false) n=93;
	else if (false) n=94;
	else if (false) n=95;
	else if (false) n=96;
	else if (false) n=97;
	else if (false) n=98;
	else if (false) n=99;
	else if (false) n=100;
	else if (false) n=101;
	else if (false) n=102;
	else if (false) n=103;
	else if (false) n=104;
	else if (false) n=105;
	else if (false) n=106;
	else if (false) n=107;
	else if (false) n=108;
	else if (false) n=109;
	else if (false) n=110;
	else if (false) n=111;
	else if (false) n=112;
	else if (false) n=113;
	else if (false) n=114;
	else if (false) n=115;
	else if (false) n=116;
	else if (false) n=117;
	else if (false) n=118;
	else if (false) n=119;
	else if (false) n=120;
	else if (false) n=121;
	else if (false) n=122;
	else if (false) n=123;
	else if (false) n=124;
	else if (false) n=125;
	else if (false) n=126;
	else if (false) n=127;
	else if (false) n=128;
	else if (false) n=129;
	else if (false) n=130;
	else if (false) n=131;
	else if (false) n=132;
	else if (false) n=133;
	else if (false) n=134;
	else if (false) n=135;
	else if (false) n=136;
	else if (false) n=137;
	else if (false) n=138;
	else if (false) n=139;
	else if (false) n=140;
	else if (false) n=141;
	else if (false) n=142;
	else if (false) n=143;
	else if (false) n=144;
	else if (false) n=145;
	else if (false) n=146;
	else if (false) n=147;
	else if (false) n=148;
	else if (false) n=149;
	else if (false) n=150;
	else if (false) n=151;
	else if (false) n=152;
	else if (false) n=153;
	else if (false) n=154;
	else if (false) n=155;
	else if (false) n=156;
	else if (false) n=157;
	else if (false) n=158;
	else if (false) n=159;
	else if (false) n=160;
	else if (false) n=161;
	else if (false) n=162;
	else if (false) n=163;
	else if (false) n=164;
	else if (false) n=165;
	else if (false) n=166;
	else if (false) n=167;
	else if (false) n=168;
	else if (false) n=169;
	else if (false) n=170;
	else if (false) n=171;
	else if (false) n=172;
	else if (false) n=173;
	else if (false) n=174;
	else if (false) n=175;
	else if (false) n=176;
	else if (false) n=177;
	else if (false) n=178;
	else if (false) n=179;
	else if (false) n=180;
	else if (false) n=181;
	else if (false) n=182;
	else if (false) n=183;
	else if (false) n=184;
	else if (false) n=185;
	else if (false) n=186;
	else if (false) n=187;
	else if (false) n=188;
	else if (false) n=189;
	else if (false) n=190;
	else if (false) n=191;
	else if (false) n=192;
	else if (false) n=193;
	else if (false) n=194;
	else if (false) n=195;
	else if (false) n=196;
	else if (false) n=197;
	else if (false) n=198;
	else if (false) n=199;
	else if (false) n=200;
	else if (false) n=201;
	else if (false) n=202;
	else if (false) n=203;
	else if (false) n=204;
	else if (false) n=205;
	else if (false) n=206;
	else if (false) n=207;
	else if (false) n=208;
	else if (false) n=209;
	else if (false) n=210;
	else if (false) n=211;
	else if (false) n=212;
	else if (false) n=213;
	else if (false) n=214;
	else if (false) n=215;
	else if (false) n=216;
	else if (false) n=217;
	else if (false) n=218;
	else if (false) n=219;
	else if (false) n=220;
	else if (false) n=221;
	else if (false) n=222;
	else if (false) n=223;
	else if (false) n=224;
	else if (false) n=225;
	else if (false) n=226;
	else if (false) n=227;
	else if (false) n=228;
	else if (false) n=229;
	else if (false) n=230;
	else if (false) n=231;
	else if (false) n=232;
	else if (false) n=233;
	else if (false) n=234;
	else if (false) n=235;
	else if (false) n=236;
	else if (false) n=237;
	else if (false) n=238;
	else if (false) n=239;
	else if (false) n=240;
	else if (false) n=241;
	else if (false) n=242;
	else if (false) n=243;
	else if (false) n=244;
	else if (false) n=245;
	else if (false) n=246;
	else if (false) n=247;
	else if (false) n=248;
	else if (false) n=249;
	else if (false) n=250;
	else if (false) n=251;
	else if (false) n=252;
	else if (false) n=253;
	else if (false) n=254;
	else if (false) n=255;
	else if (false) n=256;
	else if (false) n=257;
	else if (false) n=258;
	else if (false) n=259;
	else if (false) n=260;
	else if (false) n=261;
	else if (false) n=262;
	else if (false) n=263;
	else if (false) n=264;
	else if (false) n=265;
	else if (false) n=266;
	else if (false) n=267;
	else if (false) n=268;
	else if (false) n=269;
	else if (false) n=270;
	else if (false) n=271;
	else if (false) n=272;
	else if (false) n=273;
	else if (false) n=274;
	else if (false) n=275;
	else if (false) n=276;
	else if (false) n=277;
	else if (false) n=278;
	else if (false) n=279;
	else if (false) n=280;
	else if (false) n=281;
	else if (false) n=282;
	else if (false) n=283;
	else if (false) n=284;
	else if (false) n=285;
	else if (false) n=286;
	else if (false) n=287;
	else if (false) n=288;
	else if (false) n=289;
	else if (false) n=290;
	else if (false) n=291;
	else if (false) n=292;
	else if (false) n=293;
	else if (false) n=294;
	else if (false) n=295;
	else if (false) n=296;
	else if (false) n=297;
	else if (false) n=298;
	else if (false) n=299;
	else if (false) n=300;
	else if (false) n=301;
	else if (false) n=302;
	else if (false) n=303;
	else if (false) n=304;
	else if (false) n=305;
	else if (false) n=306;
	else if (false) n=307;
	else if (false) n=308;
	else if (false) n=309;
	else if (false) n=310;
	else if (false) n=311;
	else if (false) n=312;
	else if (false) n=313;
	else if (false) n=314;
	else if (false) n=315;
	else if (false) n=316;
	else if (false) n=317;
	else if (false) n=318;
	else if (false) n=319;
	else if (false) n=320;
	else if (false) n=321;
	else if (false) n=322;
	else if (false) n=323;
	else if (false) n=324;
	else if (false) n=325;
	else if (false) n=326;
	else if (false) n=327;
	else if (false) n=328;
	else if (false) n=329;
	else if (false) n=330;
	else if (false) n=331;
	else if (false) n=332;
	else if (false) n=333;
	else if (false) n=334;
	else if (false) n=335;
	else if (false) n=336;
	else if (false) n=337;
	else if (false) n=338;
	else if (false) n=339;
	else if (false) n=340;
	else if (false) n=341;
	else if (false) n=342;
	else if (false) n=343;
	else if (false) n=344;
	else if (false) n=345;
	else if (false) n=346;
	else if (false) n=347;
	else if (false) n=348;
	else if (false) n=349;
	else if (false) n=350;
	else if (false) n=351;
	else if (false) n=352;
	else if (false) n=353;
	else if (false) n=354;
	else if (false) n=355;
	else if (false) n=356;
	else if (false) n=357;
	else if (false) n=358;
	else if (false) n=359;
	else if (false) n=360;
	else if (false) n=361;
	else if (false) n=362;
	else if (false) n=363;
	else if (false) n=364;
	else if (false) n=365;
	else if (false) n=366;
	else if (false) n=367;
	else if (false) n=368;
	else if (false) n=369;
	else if (false) n=370;
	else if (false) n=371;
	else if (false) n=372;
	else if (false) n=373;
	else if (false) n=374;
	else if (false) n=375;
	else if (false) n=376;
	else if (false) n=377;
	else if (false) n=378;
	else if (false) n=379;
	else if (false) n=380;
	else if (false) n=381;
	else if (false) n=382;
	else if (false) n=383;
	else if (false) n=384;
	else if (false) n=385;
	else if (false) n=386;
	else if (false) n=387;
	else if (false) n=388;
	else if (false) n=389;
	else if (false) n=390;
	else if (false) n=391;
	else if (false) n=392;
	else if (false) n=393;
	else if (false) n=394;
	else if (false) n=395;
	else if (false) n=396;
	else if (false) n=397;
	else if (false) n=398;
	else if (false) n=399;
	else if (false) n=400;
	else if (false) n=401;
	else if (false) n=402;
	else if (false) n=403;
	else if (false) n=404;
	else if (false) n=405;
	else if (false) n=406;
	else if (false) n=407;
	else if (false) n=408;
	else if (false) n=409;
	else if (false) n=410;
	else if (false) n=411;
	else if (false) n=412;
	else if (false) n=413;
	else if (false) n=414;
	else if (false) n=415;
	else if (false) n=416;
	else if (false) n=417;
	else if (false) n=418;
	else if (false) n=419;
	else if (false) n=420;
	else if (false) n=421;
	else if (false) n=422;
	else if (false) n=423;
	else if (false) n=424;
	else if (false) n=425;
	else if (false) n=426;
	else if (false) n=427;
	else if (false) n=428;
	else if (false) n=429;
	else if (false) n=430;
	else if (false) n=431;
	else if (false) n=432;
	else if (false) n=433;
	else if (false) n=434;
	else if (false) n=435;
	else if (false) n=436;
	else if (false) n=437;
	else if (false) n=438;
	else if (false) n=439;
	else if (false) n=440;
	else if (false) n=441;
	else if (false) n=442;
	else if (false) n=443;
	else if (false) n=444;
	else if (false) n=445;
	else if (false) n=446;
	else if (false) n=447;
	else if (false) n=448;
	else if (false) n=449;
	else if (false) n=450;
	else if (false) n=451;
	else if (false) n=452;
	else if (false) n=453;
	else if (false) n=454;
	else if (false) n=455;
	else if (false) n=456;
	else if (false) n=457;
	else if (false) n=458;
	else if (false) n=459;
	else if (false) n=460;
	else if (false) n=461;
	else if (false) n=462;
	else if (false) n=463;
	else if (false) n=464;
	else if (false) n=465;
	else if (false) n=466;
	else if (false) n=467;
	else if (false) n=468;
	else if (false) n=469;
	else if (false) n=470;
	else if (false) n=471;
	else if (false) n=472;
	else if (false) n=473;
	else if (false) n=474;
	else if (false) n=475;
	else if (false) n=476;
	else if (false) n=477;
	else if (false) n=478;
	else if (false) n=479;
	else if (false) n=480;
	else if (false) n=481;
	else if (false) n=482;
	else if (false) n=483;
	else if (false) n=484;
	else if (false) n=485;
	else if (false) n=486;
	else if (false) n=487;
	else if (false) n=488;
	else if (false) n=489;
	else if (false) n=490;
	else if (false) n=491;
	else if (false) n=492;
	else if (false) n=493;
	else if (false) n=494;
	else if (false) n=495;
	else if (false) n=496;
	else if (false) n=497;
	else if (false) n=498;
	else if (false) n=499;
	else if (false) n=500;
	else if (false) n=501;
	else if (false) n=502;
	else if (false) n=503;
	else if (false) n=504;
	else if (false) n=505;
	else if (false) n=506;
	else if (false) n=507;
	else if (false) n=508;
	else if (false) n=509;
	else if (false) n=510;
	else if (false) n=511;
	else if (false) n=512;
	else if (false) n=513;
	else if (false) n=514;
	else if (false) n=515;
	else if (false) n=516;
	else if (false) n=517;
	else if (false) n=518;
	else if (false) n=519;
	else if (false) n=520;
	else if (false) n=521;
	else if (false) n=522;
	else if (false) n=523;
	else if (false) n=524;
	else if (false) n=525;
	else if (false) n=526;
	else if (false) n=527;
	else if (false) n=528;
	else if (false) n=529;
	else if (false) n=530;
	else if (false) n=531;
	else if (false) n=532;
	else if (false) n=533;
	else if (false) n=534;
	else if (false) n=535;
	else if (false) n=536;
	else if (false) n=537;
	else if (false) n=538;
	else if (false) n=539;
	else if (false) n=540;
	else if (false) n=541;
	else if (false) n=542;
	else if (false) n=543;
	else if (false) n=544;
	else if (false) n=545;
	else if (false) n=546;
	else if (false) n=547;
	else if (false) n=548;
	else if (false) n=549;
	else if (false) n=550;
	else if (false) n=551;
	else if (false) n=552;
	else if (false) n=553;
	else if (false) n=554;
	else if (false) n=555;
	else if (false) n=556;
	else if (false) n=557;
	else if (false) n=558;
	else if (false) n=559;
	else if (false) n=560;
	else if (false) n=561;
	else if (false) n=562;
	else if (false) n=563;
	else if (false) n=564;
	else if (false) n=565;
	else if (false) n=566;
	else if (false) n=567;
	else if (false) n=568;
	else if (false) n=569;
	else if (false) n=570;
	else if (false) n=571;
	else if (false) n=572;
	else if (false) n=573;
	else if (false) n=574;
	else if (false) n=575;
	else if (false) n=576;
	else if (false) n=577;
	else if (false) n=578;
	else if (false) n=579;
	else if (false) n=580;
	else if (false) n=581;
	else if (false) n=582;
	else if (false) n=583;
	else if (false) n=584;
	else if (false) n=585;
	else if (false) n=586;
	else if (false) n=587;
	else if (false) n=588;
	else if (false) n=589;
	else if (false) n=590;
	else if (false) n=591;
	else if (false) n=592;
	else if (false) n=593;
	else if (false) n=594;
	else if (false) n=595;
	else if (false) n=596;
	else if (false) n=597;
	else if (false) n=598;
	else if (false) n=599;
	else if (false) n=600;
	else if (false) n=601;
	else if (false) n=602;
	else if (false) n=603;
	else if (false) n=604;
	else if (false) n=605;
	else if (false) n=606;
	else if (false) n=607;
	else if (false) n=608;
	else if (false) n=609;
	else if (false) n=610;
	else if (false) n=611;
	else if (false) n=612;
	else if (false) n=613;
	else if (false) n=614;
	else if (false) n=615;
	else if (false) n=616;
	else if (false) n=617;
	else if (false) n=618;
	else if (false) n=619;
	else if (false) n=620;
	else if (false) n=621;
	else if (false) n=622;
	else if (false) n=623;
	else if (false) n=624;
	else if (false) n=625;
	else if (false) n=626;
	else if (false) n=627;
	else if (false) n=628;
	else if (false) n=629;
	else if (false) n=630;
	else if (false) n=631;
	else if (false) n=632;
	else if (false) n=633;
	else if (false) n=634;
	else if (false) n=635;
	else if (false) n=636;
	else if (false) n=637;
	else if (false) n=638;
	else if (false) n=639;
	else if (false) n=640;
	else if (false) n=641;
	else if (false) n=642;
	else if (false) n=643;
	else if (false) n=644;
	else if (false) n=645;
	else if (false) n=646;
	else if (false) n=647;
	else if (false) n=648;
	else if (false) n=649;
	else if (false) n=650;
	else if (false) n=651;
	else if (false) n=652;
	else if (false) n=653;
	else if (false) n=654;
	else if (false) n=655;
	else if (false) n=656;
	else if (false) n=657;
	else if (false) n=658;
	else if (false) n=659;
	else if (false) n=660;
	else if (false) n=661;
	else if (false) n=662;
	else if (false) n=663;
	else if (false) n=664;
	else if (false) n=665;
	else if (false) n=666;
	else if (false) n=667;
	else if (false) n=668;
	else if (false) n=669;
	else if (false) n=670;
	else if (false) n=671;
	else if (false) n=672;
	else if (false) n=673;
	else if (false) n=674;
	else if (false) n=675;
	else if (false) n=676;
	else if (false) n=677;
	else if (false) n=678;
	else if (false) n=679;
	else if (false) n=680;
	else if (false) n=681;
	else if (false) n=682;
	else if (false) n=683;
	else if (false) n=684;
	else if (false) n=685;
	else if (false) n=686;
	else if (false) n=687;
	else if (false) n=688;
	else if (false) n=689;
	else if (false) n=690;
	else if (false) n=691;
	else if (false) n=692;
	else if (false) n=693;
	else if (false) n=694;
	else if (false) n=695;
	else if (false) n=696;
	else if (false) n=697;
	else if (false) n=698;
	else if (false) n=699;
	else if (false) n=700;
	else if (false) n=701;
	else if (false) n=702;
	else if (false) n=703;
	else if (false) n=704;
	else if (false) n=705;
	else if (false) n=706;
	else if (false) n=707;
	else if (false) n=708;
	else if (false) n=709;
	else if (false) n=710;
	else if (false) n=711;
	else if (false) n=712;
	else if (false) n=713;
	else if (false) n=714;
	else if (false) n=715;
	else if (false) n=716;
	else if (false) n=717;
	else if (false) n=718;
	else if (false) n=719;
	else if (false) n=720;
	else if (false) n=721;
	else if (false) n=722;
	else if (false) n=723;
	else if (false) n=724;
	else if (false) n=725;
	else if (false) n=726;
	else if (false) n=727;
	else if (false) n=728;
	else if (false) n=729;
	else if (false) n=730;
	else if (false) n=731;
	else if (false) n=732;
	else if (false) n=733;
	else if (false) n=734;
	else if (false) n=735;
	else if (false) n=736;
	else if (false) n=737;
	else if (false) n=738;
	else if (false) n=739;
	else if (false) n=740;
	else if (false) n=741;
	else if (false) n=742;
	else if (false) n=743;
	else if (false) n=744;
	else if (false) n=745;
	else if (false) n=746;
	else if (false) n=747;
	else if (false) n=748;
	else if (false) n=749;
	else if (false) n=750;
	else if (false) n=751;
	else if (false) n=752;
	else if (false) n=753;
	else if (false) n=754;
	else if (false) n=755;
	else if (false) n=756;
	else if (false) n=757;
	else if (false) n=758;
	else if (false) n=759;
	else if (false) n=760;
	else if (false) n=761;
	else if (false) n=762;
	else if (false) n=763;
	else if (false) n=764;
	else if (false) n=765;
	else if (false) n=766;
	else if (false) n=767;
	else if (false) n=768;
	else if (false) n=769;
	else if (false) n=770;
	else if (false) n=771;
	else if (false) n=772;
	else if (false) n=773;
	else if (false) n=774;
	else if (false) n=775;
	else if (false) n=776;
	else if (false) n=777;
	else if (false) n=778;
	else if (false) n=779;
	else if (false) n=780;
	else if (false) n=781;
	else if (false) n=782;
	else if (false) n=783;
	else if (false) n=784;
	else if (false) n=785;
	else if (false) n=786;
	else if (false) n=787;
	else if (false) n=788;
	else if (false) n=789;
	else if (false) n=790;
	else if (false) n=791;
	else if (false) n=792;
	else if (false) n=793;
	else if (false) n=794;
	else if (false) n=795;
	else if (false) n=796;
	else if (false) n=797;
	else if (false) n=798;
	else if (false) n=799;
	else if (false) n=800;
	else if (false) n=801;
	else if (false) n=802;
	else if (false) n=803;
	else if (false) n=804;
	else if (false) n=805;
	else if (false) n=806;
	else if (false) n=807;
	else if (false) n=808;
	else if (false) n=809;
	else if (false) n=810;
	else if (false) n=811;
	else if (false) n=812;
	else if (false) n=813;
	else if (false) n=814;
	else if (false) n=815;
	else if (false) n=816;
	else if (false) n=817;
	else if (false) n=818;
	else if (false) n=819;
	else if (false) n=820;
	else if (false) n=821;
	else if (false) n=822;
	else if (false) n=823;
	else if (false) n=824;
	else if (false) n=825;
	else if (false) n=826;
	else if (false) n=827;
	else if (false) n=828;
	else if (false) n=829;
	else if (false) n=830;
	else if (false) n=831;
	else if (false) n=832;
	else if (false) n=833;
	else if (false) n=834;
	else if (false) n=835;
	else if (false) n=836;
	else if (false) n=837;
	else if (false) n=838;
	else if (false) n=839;
	else if (false) n=840;
	else if (false) n=841;
	else if (false) n=842;
	else if (false) n=843;
	else if (false) n=844;
	else if (false) n=845;
	else if (false) n=846;
	else if (false) n=847;
	else if (false) n=848;
	else if (false) n=849;
	else if (false) n=850;
	else if (false) n=851;
	else if (false) n=852;
	else if (false) n=853;
	else if (false) n=854;
	else if (false) n=855;
	else if (false) n=856;
	else if (false) n=857;
	else if (false) n=858;
	else if (false) n=859;
	else if (false) n=860;
	else if (false) n=861;
	else if (false) n=862;
	else if (false) n=863;
	else if (false) n=864;
	else if (false) n=865;
	else if (false) n=866;
	else if (false) n=867;
	else if (false) n=868;
	else if (false) n=869;
	else if (false) n=870;
	else if (false) n=871;
	else if (false) n=872;
	else if (false) n=873;
	else if (false) n=874;
	else if (false) n=875;
	else if (false) n=876;
	else if (false) n=877;
	else if (false) n=878;
	else if (false) n=879;
	else if (false) n=880;
	else if (false) n=881;
	else if (false) n=882;
	else if (false) n=883;
	else if (false) n=884;
	else if (false) n=885;
	else if (false) n=886;
	else if (false) n=887;
	else if (false) n=888;
	else if (false) n=889;
	else if (false) n=890;
	else if (false) n=891;
	else if (false) n=892;
	else if (false) n=893;
	else if (false) n=894;
	else if (false) n=895;
	else if (false) n=896;
	else if (false) n=897;
	else if (false) n=898;
	else if (false) n=899;
	else if (false) n=900;
	else if (false) n=901;
	else if (false) n=902;
	else if (false) n=903;
	else if (false) n=904;
	else if (false) n=905;
	else if (false) n=906;
	else if (false) n=907;
	else if (false) n=908;
	else if (false) n=909;
	else if (false) n=910;
	else if (false) n=911;
	else if (false) n=912;
	else if (false) n=913;
	else if (false) n=914;
	else if (false) n=915;
	else if (false) n=916;
	else if (false) n=917;
	else if (false) n=918;
	else if (false) n=919;
	else if (false) n=920;
	else if (false) n=921;
	else if (false) n=922;
	else if (false) n=923;
	else if (false) n=924;
	else if (false) n=925;
	else if (false) n=926;
	else if (false) n=927;
	else if (false) n=928;
	else if (false) n=929;
	else if (false) n=930;
	else if (false) n=931;
	else if (false) n=932;
	else if (false) n=933;
	else if (false) n=934;
	else if (false) n=935;
	else if (false) n=936;
	else if (false) n=937;
	else if (false) n=938;
	else if (false) n=939;
	else if (false) n=940;
	else if (false) n=941;
	else if (false) n=942;
	else if (false) n=943;
	else if (false) n=944;
	else if (false) n=945;
	else if (false) n=946;
	else if (false) n=947;
	else if (false) n=948;
	else if (false) n=949;
	else if (false) n=950;
	else if (false) n=951;
	else if (false) n=952;
	else if (false) n=953;
	else if (false) n=954;
	else if (false) n=955;
	else if (false) n=956;
	else if (false) n=957;
	else if (false) n=958;
	else if (false) n=959;
	else if (false) n=960;
	else if (false) n=961;
	else if (false) n=962;
	else if (false) n=963;
	else if (false) n=964;
	else if (false) n=965;
	else if (false) n=966;
	else if (false) n=967;
	else if (false) n=968;
	else if (false) n=969;
	else if (false) n=970;
	else if (false) n=971;
	else if (false) n=972;
	else if (false) n=973;
	else if (false) n=974;
	else if (false) n=975;
	else if (false) n=976;
	else if (false) n=977;
	else if (false) n=978;
	else if (false) n=979;
	else if (false) n=980;
	else if (false) n=981;
	else if (false) n=982;
	else if (false) n=983;
	else if (false) n=984;
	else if (false) n=985;
	else if (false) n=986;
	else if (false) n=987;
	else if (false) n=988;
	else if (false) n=989;
	else if (false) n=990;
	else if (false) n=991;
	else if (false) n=992;
	else if (false) n=993;
	else if (false) n=994;
	else if (false) n=995;
	else if (false) n=996;
	else if (false) n=997;
	else if (false) n=998;
	else if (false) n=999;
	else if (false) n=1000;
	else if (false) n=1001;
	else if (false) n=1002;
	else if (false) n=1003;
	else if (false) n=1004;
	else if (false) n=1005;
	else if (false) n=1006;
	else if (false) n=1007;
	else if (false) n=1008;
	else if (false) n=1009;
	else if (false) n=1010;
	else if (false) n=1011;
	else if (false) n=1012;
	else if (false) n=1013;
	else if (false) n=1014;
	else if (false) n=1015;
	else if (false) n=1016;
	else if (false) n=1017;
	else if (false) n=1018;
	else if (false) n=1019;
	else if (false) n=1020;
	else if (false) n=1021;
	else if (false) n=1022;
	else if (false) n=1023;
	else if (false) n=1024;
	else if (false) n=1025;
	else if (false) n=1026;
	else if (false) n=1027;
	else if (false) n=1028;
	else if (false) n=1029;
	else if (false) n=1030;
	else if (false) n=1031;
	else if (false) n=1032;
	else if (false) n=1033;
	else if (false) n=1034;
	else if (false) n=1035;
	else if (false) n=1036;
	else if (false) n=1037;
	else if (false) n=1038;
	else if (false) n=1039;
	else if (false) n=1040;
	else if (false) n=1041;
	else if (false) n=1042;
	else if (false) n=1043;
	else if (false) n=1044;
	else if (false) n=1045;
	else if (false) n=1046;
	else if (false) n=1047;
	else if (false) n=1048;
	else if (false) n=1049;
	else if (false) n=1050;
	else if (false) n=1051;
	else if (false) n=1052;
	else if (false) n=1053;
	else if (false) n=1054;
	else if (false) n=1055;
	else if (false) n=1056;
	else if (false) n=1057;
	else if (false) n=1058;
	else if (false) n=1059;
	else if (false) n=1060;
	else if (false) n=1061;
	else if (false) n=1062;
	else if (false) n=1063;
	else if (false) n=1064;
	else if (false) n=1065;
	else if (false) n=1066;
	else if (false) n=1067;
	else if (false) n=1068;
	else if (false) n=1069;
	else if (false) n=1070;
	else if (false) n=1071;
	else if (false) n=1072;
	else if (false) n=1073;
	else if (false) n=1074;
	else if (false) n=1075;
	else if (false) n=1076;
	else if (false) n=1077;
	else if (false) n=1078;
	else if (false) n=1079;
	else if (false) n=1080;
	else if (false) n=1081;
	else if (false) n=1082;
	else if (false) n=1083;
	else if (false) n=1084;
	else if (false) n=1085;
	else if (false) n=1086;
	else if (false) n=1087;
	else if (false) n=1088;
	else if (false) n=1089;
	else if (false) n=1090;
	else if (false) n=1091;
	else if (false) n=1092;
	else if (false) n=1093;
	else if (false) n=1094;
	else if (false) n=1095;
	else if (false) n=1096;
	else if (false) n=1097;
	else if (false) n=1098;
	else if (false) n=1099;
	else if (false) n=1100;
	else if (false) n=1101;
	else if (false) n=1102;
	else if (false) n=1103;
	else if (false) n=1104;
	else if (false) n=1105;
	else if (false) n=1106;
	else if (false) n=1107;
	else if (false) n=1108;
	else if (false) n=1109;
	else if (false) n=1110;
	else if (false) n=1111;
	else if (false) n=1112;
	else if (false) n=1113;
	else if (false) n=1114;
	else if (false) n=1115;
	else if (false) n=1116;
	else if (false) n=1117;
	else if (false) n=1118;
	else if (false) n=1119;
	else if (false) n=1120;
	else if (false) n=1121;
	else if (false) n=1122;
	else if (false) n=1123;
	else if (false) n=1124;
	else if (false) n=1125;
	else if (false) n=1126;
	else if (false) n=1127;
	else if (false) n=1128;
	else if (false) n=1129;
	else if (false) n=1130;
	else n=1804;

    if ( n != 1804 ) apLogFailInfo( "deep nesting failed ", 1804, n,"");

    apEndTest();

}

if019();


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
