/*
 * country_codes.h - Country name to ISO 3166-1 alpha-2 lookup + flag emoji helper
 *
 * Returns a JavaScript snippet that defines:
 *   countryISO  — object mapping full country names to 2-letter ISO codes
 *   countryFlag(name) — converts a country name to its Unicode flag emoji
 *
 * Usage in a page:  html += getCountryFlagJS();
 */

#ifndef WEB_COUNTRY_CODES_H
#define WEB_COUNTRY_CODES_H

inline String getCountryFlagJS() {
  String js = "var countryISO={";
  // A
  js += "'Afghanistan':'AF','Albania':'AL','Algeria':'DZ','Andorra':'AD','Angola':'AO',";
  js += "'Anguilla':'AI','Antigua and Barbuda':'AG','Antigua And Barbuda':'AG',";
  js += "'Argentina':'AR','Argentina Republic':'AR','Armenia':'AM','Aruba':'AW',";
  js += "'Aland Islands':'AX','Aaland Islands':'AX','American Samoa':'AS','Ascension Island':'SH',";
  js += "'Australia':'AU','Austria':'AT','Azerbaijan':'AZ',";
  // B
  js += "'Bahamas':'BS','Bahrain':'BH','Bangladesh':'BD','Barbados':'BB','Belarus':'BY',";
  js += "'Belgium':'BE','Belize':'BZ','Benin':'BJ','Bermuda':'BM','Bhutan':'BT',";
  js += "'Bolivia':'BO','Bonaire':'BQ','Caribbean Netherlands':'BQ',";
  js += "'Bosnia and Herzegovina':'BA','Bosnia And Hercegovina':'BA','Bosnia':'BA','Botswana':'BW',";
  js += "'Brazil':'BR','British Indian Ocean Territory':'IO','British Virgin Islands':'VG',";
  js += "'Brunei':'BN','Brunei Darussalam':'BN','Bulgaria':'BG','Burkina Faso':'BF','Burundi':'BI',";
  // C
  js += "'Cabo Verde':'CV','Cape Verde':'CV','Cambodia':'KH','Cameroon':'CM','Canada':'CA',";
  js += "'Cayman Islands':'KY','Central African Republic':'CF','Chad':'TD','Chile':'CL',";
  js += "'China':'CN','Christmas Island':'CX','Cocos Islands':'CC','Cocos Keeling Islands':'CC',";
  js += "'Colombia':'CO','Comoros':'KM','Congo':'CG','Cook Islands':'CK','Corsica':'FR','Costa Rica':'CR',";
  js += "'Cote d Ivoire':'CI','Croatia':'HR','Cuba':'CU','Curacao':'CW','Cyprus':'CY',";
  js += "'Czechia':'CZ','Czech Republic':'CZ',";
  // D
  js += "'Democratic Republic of the Congo':'CD','Denmark':'DK','Djibouti':'DJ',";
  js += "'Dominica':'DM','Dominican Republic':'DO',";
  // E
  js += "'Ecuador':'EC','Egypt':'EG','El Salvador':'SV','Equatorial Guinea':'GQ',";
  js += "'Eritrea':'ER','Estonia':'EE','Eswatini':'SZ','Swaziland':'SZ','Swasiland':'SZ','Ethiopia':'ET',";
  // F
  js += "'Falkland Islands':'FK','Faroe Islands':'FO','Fiji':'FJ','Finland':'FI','France':'FR',";
  js += "'French Guiana':'GF','French Polynesia':'PF','French Southern Territories':'TF',";
  // G
  js += "'Gabon':'GA','Gambia':'GM','Georgia':'GE','Germany':'DE','Ghana':'GH',";
  js += "'Gibraltar':'GI','Greece':'GR','Greenland':'GL','Grenada':'GD','Guadeloupe':'GP','Guam':'GU',";
  js += "'Guatemala':'GT','Guernsey':'GG','Guinea':'GN','Guinea-Bissau':'GW','Guyana':'GY',";
  // H
  js += "'Haiti':'HT','Honduras':'HN','Hong Kong':'HK','Hungary':'HU',";
  // I
  js += "'Iceland':'IS','India':'IN','Indonesia':'ID','Iran':'IR','Iraq':'IQ',";
  js += "'Ireland':'IE','Isle of Man':'IM','Israel':'IL','Italy':'IT','Ivory Coast':'CI',";
  // J
  js += "'Jamaica':'JM','Japan':'JP','Jersey':'JE','Jordan':'JO',";
  // K
  js += "'Kazakhstan':'KZ','Kenya':'KE','Kiribati':'KI','Kosovo':'XK','Kuwait':'KW','Kyrgyzstan':'KG',";
  // L
  js += "'Laos':'LA','Latvia':'LV','Lebanon':'LB','Lesotho':'LS','Liberia':'LR',";
  js += "'Libya':'LY','Liechtenstein':'LI','Lithuania':'LT','Luxembourg':'LU','Luxemburg':'LU',";
  // M
  js += "'Macau':'MO','Macao':'MO','Macedonia':'MK','Madagascar':'MG','Malawi':'MW',";
  js += "'Malaysia':'MY','Maldives':'MV','Mali':'ML',";
  js += "'Malta':'MT','Marshall Islands':'MH','Martinique':'MQ','Mauritania':'MR','Mauretania':'MR',";
  js += "'Mauritius':'MU','Mayotte':'YT','Mexico':'MX','Micronesia':'FM','Moldova':'MD','Monaco':'MC',";
  js += "'Mongolia':'MN','Montserrat':'MS','Montenegro':'ME','Morocco':'MA','Mozambique':'MZ','Myanmar':'MM',";
  // N
  js += "'Namibia':'NA','Nauru':'NR','Nepal':'NP','Netherlands':'NL','New Caledonia':'NC',";
  js += "'New Zealand':'NZ','Nicaragua':'NI','Niger':'NE','Nigeria':'NG','Niue':'NU',";
  js += "'Norfolk Island':'NF','North Korea':'KP','North Macedonia':'MK','Northern Mariana Islands':'MP','Norway':'NO',";
  // O
  js += "'Oman':'OM',";
  // P
  js += "'Pakistan':'PK','Palau':'PW','Palestine':'PS','Panama':'PA','Papua New Guinea':'PG',";
  js += "'Paraguay':'PY','Peru':'PE','Philippines':'PH','Pitcairn Islands':'PN','Poland':'PL','Portugal':'PT',";
  js += "'Puerto Rico':'PR',";
  // Q
  js += "'Qatar':'QA',";
  // R
  js += "'Reunion':'RE','Romania':'RO','Russia':'RU','Rwanda':'RW',";
  // S
  js += "'Saint Barthelemy':'BL','Saint Helena':'SH','Saint Kitts and Nevis':'KN','Saint Kitts And Nevis':'KN','Saint Lucia':'LC',";
  js += "'Saint Martin':'MF','Saint Pierre and Miquelon':'PM','Saint Vincent and the Grenadines':'VC','Saint Vincent And The Grenadines':'VC',";
  js += "'Samoa':'WS','San Marino':'SM','Sao Tome and Principe':'ST','Saudi Arabia':'SA','Senegal':'SN','Serbia':'RS',";
  js += "'Seychelles':'SC','Sierra Leone':'SL','Singapore':'SG','Sint Maarten':'SX','Slovakia':'SK',";
  js += "'Slovenia':'SI','Solomon Islands':'SB','Somalia':'SO','South Africa':'ZA',";
  js += "'South Georgia':'GS','South Korea':'KR','Korea':'KR','Korea Republic Of':'KR','Korea Republic of':'KR','South Sudan':'SS','Spain':'ES','Sri Lanka':'LK',";
  js += "'Sudan':'SD','Suriname':'SR','Svalbard':'SJ','Sweden':'SE','Switzerland':'CH','Syria':'SY',";
  // T
  js += "'Taiwan':'TW','Tajikistan':'TJ','Tanzania':'TZ','Thailand':'TH',";
  js += "'Timor-Leste':'TL','East Timor':'TL','Togo':'TG','Tokelau':'TK','Tonga':'TO',";
  js += "'Trinidad and Tobago':'TT','Trinidad And Tobago':'TT',";
  js += "'Tunisia':'TN','Turkey':'TR','Turkiye':'TR','Turkmenistan':'TM','Turks and Caicos Islands':'TC','Tuvalu':'TV',";
  // U
  js += "'Uganda':'UG','Ukraine':'UA','United Arab Emirates':'AE',";
  js += "'United Kingdom':'GB','United States':'US','United States Virgin Islands':'VI','U.S. Virgin Islands':'VI','Uruguay':'UY','Uzbekistan':'UZ',";
  // V
  js += "'Vanuatu':'VU','Vatican':'VA','Venezuela':'VE','Vietnam':'VN','Virgin Islands':'VI',";
  // W
  js += "'Wallis and Futuna':'WF','Western Sahara':'EH',";
  // Y-Z
  js += "'Yemen':'YE','Zambia':'ZM','Zimbabwe':'ZW'";
  js += "};";
  js += "function countryFlag(name){";
  js += "var iso=countryISO[name];if(!iso)return '\xF0\x9F\x8C\x8D';";
  js += "return iso.split('').map(function(c){return String.fromCodePoint(c.charCodeAt(0)+0x1F1A5);}).join('');}";
  return js;
}

#endif // WEB_COUNTRY_CODES_H
