#include "search_string_utils.hpp"

#include "std/set.hpp"
#include "std/transform_iterator.hpp"

#include "base/macros.hpp"

char const * STREET_TOKENS_SEPARATOR = "\t -,.";

strings::UniString search::FeatureTypeToString(uint32_t type)
{
  string const s = "!type:" + strings::to_string(type);
  return strings::UniString(s.begin(), s.end());
}

/// @todo Move prefixes, suffixes into separate file (autogenerated).
/// "Набережная" улица встречается в городах

char const * affics[] =
{
  // Russian
  "аллея", "бульвар", "набережная", "переулок", "площадь", "проезд", "проспект", "шоссе", "тупик", "улица", "тракт", "ал", "бул", "наб", "пер", "пл", "пр", "просп", "ш", "туп", "ул", "тр",

  // English
  "street", "avenue", "square", "road", "boulevard", "drive", "highway", "lane", "way", "close", "court", "crescent", "industrial", "st", "av", "ave", "sq", "rd", "blvd", "dr", "hwy", "ln", "cl", "ct", "cres", "ind", "wy",

  // German
  "strasse", "weg", "platz",

  // Lithuanian
  "g", "pr", "pl", "kel",

  // Български език - Bulgarian
  "Блок", "бл", "Булевард", "Вход", "вх", "Генерал", "ген", "Град", "гр", "Доктор", "д-р", "Доцент", "доц", "Капитан", "кап", "Митрополит", "мит", "Площад", "Професор", "проф", "Свети", "Св", "Улица", "Село", "с", "Квартал", "кв", "Жилищен Комплекс", "ж.к",

  // Canada - Canada
  "abbey", "acres", "allée", "allÉe", "alley", "autoroute", "aut", "bay", "beach", "bend", "boul", "by-pass", "bypass", "byway", "campus", "cape", "carré", "car", "carrefour", "carref", "centre", "ctr", "c", "cercle", "chase", "chemin", "ch", "circle", "cir", "circuit", "circt", "common", "concession", "conc", "corners", "crnrs", "côte", "cÔte", "cour", "cours", "crt", "cove", "croissant", "crois", "crossing", "cross", "cul-de-sac", "cds", "dale", "dell", "diversion", "divers", "downs", "Échangeur", "Éch", "end", "esplanade", "espl", "estates", "estate", "expressway", "expy", "extension", "exten", "farm", "field", "forest", "freeway", "fwy", "front", "gardens", "gdns", "gate", "glade", "glen", "green", "grounds", "grnds", "grove", "harbour", "harbr", "heath", "heights", "hts", "highlands", "hghlds", "hill", "hollow", "Île", "impasse", "imp", "inlet", "island", "key", "knoll", "landing", "landng", "limits", "lmts", "line", "link", "lookout", "lkout", "loop", "mall", "manor", "maze", "meadow", "mews", "montée", "montÉe", "moor", "mount", "mountain", "mtn", "orchard", "orch", "parade", "parc", "park", "pk", "parkway", "pky", "passage", "pass", "path", "pathway", "ptway", "pines", "place", "plateau", "plat", "plaza", "point", "pt", "pointe", "port", "private", "pvt", "promenade", "prom", "quai", "quay", "ramp", "rang", "range", "rg", "ridge", "rise", "rond-point", "rdpt", "route", "rte", "row", "rue", "ruelle", "rle", "run", "sentier", "sent", "subdivision", "subdiv", "terrace", "terr", "terrasse", "tsse", "thicket", "thick", "towers", "townline", "tline", "trail", "turnabout", "trnabt", "vale", "via", "view", "village", "villge", "villas", "vista", "voie", "walk", "wharf", "wood", "wynd",

  // Cesky - Czech
  "ulice", "ul", "náměstí", "nám",

  // Dansk - Danish
  "lille", "ll", "nordre", "ndr", "søndre", "sdr", "store", "gammel", "gl",

  // Deutsch - German
  "am", "a", "an der", "a.d", "auf der", "allee", "al", "berg", "bg", "burg", "bahnhof", "bhf", "bf", "brücke", "br", "bürgermeister", "bgm", "chaussee", "dorf", "df", "damm", "d", "doktor", "gasse", "großer", "große", "großes", "gr", "hauptbahnhof", "hbf", "heiligen", "hl", "hof", "h", "im", "in", "in der", "kamp", "k", "kleiner", "kleine", "kleines", "kl", "ob", "oberer", "obere", "oberes", "pfad", "p", "quelle", "qu", "rhein", "rh", "sankt", "straße", "str", "unterer", "untere", "unteres", "von", "von der", "vor der",

  // Español - Spanish
  "acceso", "acces", "acequia", "aceq", "alameda", "alam", "alquería", "alque", "andador", "andad", "angosta", "angta", "apartamentos", "aptos", "apeadero", "apdro", "arboleda", "arb", "arrabal", "arral", "arroyo", "arry", "autopista", "auto", "autovía", "autov", "avenida", "avd", "avda", "bajada", "bjada", "balneario", "balnr", "banda", "b", "bda", "barranco", "branc", "barranquil", "bqllo", "barriada", "barda", "barrio", "barro", "bo", "bloque", "blque", "brazal", "brzal", "bulevar", "bulev", "calle", "c/", "calleja", "cllja", "callejón", "callej", "cjon", "cllon", "callejuela", "cjla", "callizo", "cllzo", "calzada", "czada", "camino", "cno", "cmno", "camino hondo", "c.h", "camino nuevo", "c.n", "camino viejo", "c.v", "camping", "campg", "cantera", "cantr", "cantina", "canti", "cantón", "cant", "carrera", "cra", "carrero", "cro", "carretera", "ctra", "carreterín", "ctrin", "carretil", "crtil", "carril", "crril", "caserío", "csrio", "chalet", "chlet", "cinturón", "cint", "circunvalación", "ccvcn", "cobertizo", "cbtiz", "colonia", "col", "complejo", "compj", "conjunto", "cjto", "convento", "cnvto", "cooperativa", "coop", "corral", "crral", "corralillo", "crrlo", "corredor", "crrdo", "cortijo", "crtjo", "costanilla", "cstan", "costera", "coste", "cuadra", "cuadr", "cuesta", "custa", "dehesa", "dhsa", "demarcación", "demar", "diagonal", "diag", "diseminado", "disem", "edificio", "edifc", "empresa", "empr", "entrada", "entd", "escalera", "esca", "escalinata", "escal", "espalda", "eslda", "estación", "estcn", "estrada", "estda", "explanada", "expla", "extramuros", "extrm", "extrarradio", "extrr", "fábrica", "fca", "fbrca", "galería", "gale", "glorieta", "gta", "gran vía", "g.v", "granja", "granj", "hipódromo", "hipod", "jardín", "jdin", "ladera", "ldera", "llanura", "llnra", "malecón", "malec", "mercado", "merc", "mirador", "mrdor", "monasterio", "mtrio", "muelle", "muell", "núcleo", "ncleo", "palacio", "palac", "pantano", "pant", "paraje", "praje", "parque", "pque", "particular", "parti", "partida", "ptda", "pasadizo", "pzo", "pasaje", "psaje", "paseo", "pº", "paseo marítimo", "psmar", "pasillo", "psllo", "plza", "pza", "plazoleta", "pzta", "plazuela", "plzla", "poblado", "pbdo", "polígono", "polig", "polígono industrial", "pgind", "p.i", "portal", "prtal", "pórtico", "prtco", "portillo", "ptilo", "prazuela", "przla", "prolongación", "prol", "pueblo", "pblo", "puente", "pnte", "puerta", "pta", "puerto", "pto", "punto kilométrico", "p.k", "rambla", "rbla", "rampla", "rampa", "residencial", "resid", "ribera", "rbra", "rincón", "rcon", "rinconada", "rcda", "rotonda", "rtda", "san", "s", "sanatorio", "sanat", "santa", "sta", "santuario", "santu", "sector", "sect", "sendera", "sedra", "sendero", "send", "subida", "sbida", "torrente", "trrnt", "tránsito", "trans", "transversal", "trval", "trasera", "tras", "travesía", "trva", "urbanización", "urb", "vecindario", "vecin", "vereda", "vreda", "viaducto", "vcto", "viviendas", "vvdas",

  // Français - French
  "abbaye", "abe", "agglomération", "agl", "aire", "aires", "all", "allées", "ancien chemin", "ach", "ancienne route", "art", "anciennes routes", "anse", "arcade", "arc", "arcades", "barrière", "bre", "barrières", "bas chemin", "bch", "bastide", "bstd", "baston", "bast", "béguinage", "begi", "béguinages", "berge", "ber", "berges", "bois", "boucle", "bcle", "bd", "bourg", "brg", "butte", "but", "cité", "cite", "cités", "cote", "côteau", "cale", "camp", "campagne", "cgne", "cpg", "carreau", "cau", "carrière", "care", "carrières", "carr", "castel", "cst", "cavée", "cav", "central", "ctre", "chl", "chapelle", "chp", "charmille", "chi", "chaussée", "chs", "chaussées", "che", "chemin vicinal", "chv", "cheminement", "chem", "cheminements", "chemins", "chemins vicinaux", "chez", "château", "cht", "cloître", "cloi", "clos", "colline", "coli", "collines", "contour", "corniche", "cor", "corniches", "cottage", "cott", "cottages", "crs", "darse", "dars", "degré", "deg", "degrés", "descente", "dsg", "descentes", "digue", "dig", "digues", "domaine", "dom", "domaines", "Écluse", "ecl", "Écl", "Écluses", "Église", "egl", "Égl", "enceinte", "en", "enclave", "env", "enclos", "enc", "escalier", "esc", "escaliers", "espace", "espa", "esp", "esplanades", "Étang", "etang", "faubourg", "fg", "ferme", "frm", "fermes", "fontaine", "fon", "fort", "forum", "form", "fosse", "fos", "fosses", "foyer", "foyr", "galerie", "gal", "galeries", "gare", "garenne", "garn", "grand boulevard", "gbd", "grand ensemble", "gden", "grand’rue", "grande rue", "grandes rues", "grands ensembles", "grille", "gri", "grimpette", "grim", "groupe", "gpe", "groupement", "gpt", "groupes", "halle", "hle", "halles", "hameau", "ham", "hameaux", "haut chemin", "hch", "hauts chemins", "hippodrome", "hip", "hlm", "ile", "immeuble", "imm", "immeubles", "impasses", "jardin", "jard", "jardins", "jetée", "jte", "jetées", "levée", "leve", "lieu-dit", "ld", "lotissement", "lot", "lotissements", "mail", "maison forestière", "mf", "manoir", "man", "marche", "mar", "marches", "mas", "monseigneur", "mgr", "mont", "mt", "mte", "montées", "moulin", "mln", "moulins", "musée", "mus", "métro", "met", "mÉt", "nouvelle route", "nte", "palais", "pal", "parcs", "parking", "pkg", "parvis", "prv", "pas", "passage à niveau", "pn", "passe", "passerelle", "ple", "passerelles", "passes", "patio", "pat", "pavillon", "pav", "pavillons", "petit chemin", "pch", "petite allée", "petite avenue", "pae", "petite impasse", "pim", "petite route", "prt", "petite rue", "ptr", "petites allées", "placis", "plci", "plage", "plag", "plages", "plaine", "pln", "plan", "plt", "plateaux", "pnt", "pont", "ponts", "porche", "porte", "pte", "portique", "porq", "portiques", "poterne", "pot", "pourtour", "pour", "presqu’île", "prq", "pré", "pre", "prÉ", "périphérique", "peri", "péristyle", "psty", "quartier", "qua", "raccourci", "rac", "raidillon", "raid", "rampe", "rpe", "rempart", "rem", "roc", "rocade", "rond point", "rpt", "roquet", "roqt", "rotonde", "rtd", "routes", "r", "ruelles", "rues", "résidence", "res", "résidences", "saint", "sainte", "ste", "sente", "sen", "sentes", "sentiers", "stade", "stde", "station", "terrain", "trn", "terrasses", "terre plein", "tpl", "tertre", "trt", "tertres", "tour", "traverse", "tra", "vallon", "val", "vallée", "venelle", "ven", "venelles", "vieille route", "vte", "vieux chemin", "vche", "villa", "vla", "vge", "villages", "voi", "voies", "zone", "zone artisanale", "za", "zone d'aménagement concerté", "zac", "zone d'aménagement différé", "zad", "zone industrielle", "zi", "zone à urbaniser en priorité", "zup",

  // Nederlands - Dutch
  "broeder", "burgemeester", "commandant", "cmdt", "doctor", "dokter", "dominee", "ds", "gebroeders", "gebr", "generaal", "gen", "gracht", "ingenieur", "ir.", "ir", "jonkheer", "jhr", "kolonel", "kol", "kanunnik", "kan", "kardinaal", "kard", "kort(e)", "kte.", "koning", "kon", "koningin", "laan", "ln.", "lange", "l", "luitenant", "luit", "markt", "mkt", "meester", "mr.", "mr", "mevrouw", "mevr", "mgr.", "onze-lieve-vrouw(e)-", "o.l.v.-", "o.l.v", "olv-", "olv", "pastoor", "past", "plein", "pln.", "president", "pres", "prins", "prinses", "professor", "prof", "straat", "steenweg", "stwg", "sint-", "st.-", "st.", "van", "v", "van de", "v. d", "vd", "vliet", "vlt",

  // Norsk - Norwegian
  "vei", "veien", "vn", "gaten", "gata", "gt", "plass", "plassen", "sving", "svingen", "sv",

  // Polski - Polish
  "aleja", "aleje", "aleji", "alejach", "aleją", "dolny", "dolna", "dolne", "dln", "drugi", "druga", "drugie", "ii", "duży", "duża", "duże", "dz", "dż", "górny", "górna", "górne", "grn", "kolonia", "koło", "kolo", "mały", "mała", "małe", "ml", "mł", "mazowiecka", "mazowiecki", "mazowieckie", "maz", "miasto", "m", "nowy", "nowa", "nowe", "nw", "n", "osiedle", "osiedlu", "os", "no", "pierwszy", "pierwsza", "pierwsze", "plac", "placu", "placem", "stary", "stara", "stare", "Świętego", "Świętej", "Świętych", "św", "trzeci", "trzecia", "trzecie", "iii", "ulica", "ulicą", "ulicy", "wielki", "wielka", "wielkie", "wlk", "wielkopolski", "wielkopolska", "wielkopolskie", "wlkp", "województwo", "województwie", "woj",

  // Português - Portuguese
  "associação", "ass", "alferes", "alf", "almirante", "alm", "arquitecto", "arq", "arqº", "arquiteto", "auto-estrada", "avª", "azinhaga", "az", "bairro", "bº", "beco", "bc", "bco", "bloco", "bl", "bombeiros voluntários", "bv", "b.v", "brigadeiro", "cacique", "cac", "calçada", "cc", "calçadinha", "ccnh", "câmara municipal", "cm", "c.m", "c. m", "caminho", "cam", "capitão", "cap", "casal", "csl", "cave", "cv", "centro comercial", "c.c", "ciclo do ensino básico", "ceb", "c.e.b", "c. e. b", "comandante", "comendador", "comend", "companhia", "cª", "conselheiro", "cons", "coronel", "cel", "de", "d´", "d'", "deputado", "dep", "direito", "dto", "dona", "dª", "doutor", "doutora", "drª", "dra", "duque", "dq", "edifício", "ed", "edf", "embaixador", "emb", "empresa pública", "ep", "e.p", "enfermeiro", "enfo", "enfº", "enf", "engenheiro", "eng", "engº", "engenheira", "engª", "escadas", "escadinhas", "escnh", "escola básica", "eb", "e.b", "e. b", "esquerdo", "esq", "estação de tratamento de Águas residuais", "etar", "e.t.a.r", "estr", "estrada municipal", "em", "estrada nacional", "estrada regional", "er", "frei", "fr", "frente", "ft", "futebol clube", "fc", "f.c", "f. c", "guarda nacional republicana", "gnr", "general", "habitação", "hab", "infante", "inf", "instituto", "inst", "irmã", "ima", "imª", "irmão", "imo", "imº", "itinerário complementar", "ic", "itinerário principal", "ip", "jardim", "jrd", "júnior", "jr", "largo", "lg", "limitada", "lda", "loja", "lj", "lote", "lt", "loteamento", "loteam", "lugar", "lug", "maestro", "mto", "major", "maj", "marechal", "mal", "marquês", "mq", "madre", "me", "mestre", "ministério", "min", "monsenhor", "mons", "municipal", "nacional", "nossa", "nª", "nossa senhora", "ns", "nosso", "número", "nº", "padre", "pe", "pq", "part", "pátio", "pavilhão", "polícia de segurança pública", "psp", "p.s.p", "polícia judiciária", "pj", "p.j", "praça", "pc", "pç", "praceta", "pct", "pctª", "presidente", "presid", "primeiro", "1º", "professora", "profª", "projectada", "proj", "projetada", "prolongamento", "prolng", "quadra", "q", "qd", "quinta", "qta", "regional", "rés-do-chão", "r/c", "rc", "rotunda", "rot", "ribeira", "rª", "rib", "ribª", "rio", "rua", "stª", "santo", "sto", "stº", "são", "sargento", "sarg", "sem número", "s/n", "sn", "senhor", "sr", "senhora", "sª", "srª", "sr.ª", "s.ra", "sra", "sobre-loja", "slj", "sociedade", "soc", "sociedade anónima", "sa", "s.a", "sport clube", "sc", "sub-cave", "scv", "superquadra", "tenente", "ten", "torre", "tr", "transv", "travessa", "trav", "trv", "tv", "universidade", "univ", "urbanização", "vila", "vl", "visconde", "visc", "vivenda", "vv", "zona", "zn",

  // Română - Romanian
  "aleea", "ale", "alea", "bulevardul", "bul", "bdul", "blv", "b-dul", "b.dul", "bulevard", "bulevardu", "calea", "cal", "fundătura", "fnd", "fundacul", "fdc", "intrarea", "int", "intr", "piața", "pţa", "pța", "p-ta", "p-ţa", "p-ța", "piaţa", "piață", "piaţă", "piată", "piata", "strada", "stra", "stradela", "str-la", "sdla", "Șoseaua", "sos", "soseaua", "splaiul", "sp", "spl", "vârful", "vf", "virful", "vîrful", "varful", "virf", "varf", "muntele", "m-tele", "m-te", "mnt",

  // Slovenščina - Slovenian
  "cesta", "spodnja", "spodnje", "spodnji", "srednja", "srednje", "srednji", "sveta", "sveti", "velika", "velike", "veliki", "veliko", "zgornja", "zg", "zgornje", "zgornji",

  // Suomi - Finnish
  "alue", "aukio", "auk", "kaari", "kri", "katu", "kuja", "kj", "kylä", "penger", "pgr", "polku", "puistikko", "pko", "puisto", "ps", "raitti", "ranta", "rt", "rinne", "rn", "taival", "tvl", "tie", "t", "tori", "väylä", "vlä",

  // Svenska - Swedish
  "väg", "vägen", "gatan", "gränd", "gränden", "gamla", "g:la", "södra", "s:a", "norra", "n:a", "östra", "västra", "v:a", "stig", "stigen", "sankta", "plats", "platsen", "lilla", "stora",

  // Türkçe - Turkish
  "sokak", "sk", "sok", "sokağı", "cadde", "cd", "caddesi", "bulvar", "bulvarı",

  // Tiếng Việt – Vietnamese
  "thành phố", "tp", "thị xã", "tx", "thị trấn", "tt", "quận", "phường", "ph", "quốc lộ", "ql", "tỉnh lộ", "tl", "Đại lộ", "Đl", "Đường", "Đ", "công trường", "quảng trường", "qt", "sân bay", "sb", "sân bay quốc tế", "sbqt", "phi trường", "Đường sắt", "Đs", "trung tâm", "trung tâm thương mại", "tttm", "khách sạn", "ks", "k/s", "bưu điện", "bĐ", "Đại học", "Đh", "cao đẳng", "cĐ", "trung học phổ thông", "thpt", "trung học cơ sở", "thcs", "tiểu học", "th", "khu công nghiệp", "kcn", "khu nghỉ mát", "knm", "khu du lịch", "kdl", "công viên văn hóa", "cvvh", "công viên", "vươn quốc gia", "vqg", "viện bảo tàng", "vbt", "sân vận động", "svĐ", "nhà thi đấu", "ntĐ", "câu lạc bộ", "clb", "nhà thờ", "nt", "nhà hát", "nh", "rạp hát", "công ty", "cty", "tổng công ty", "tcty", "tct", "công ty cổ phần", "ctcp", "cty cp", "căn cứ không quân", "cckq",

  // Українська - Ukrainian
  "дорога", "провулок", "площа", "шосе", "вулиция", "дор", "пров", "вул"
};

void search::GetStreetName(strings::SimpleTokenizer iter, string & streetName)
{
  while (iter)
  {
    string const s = strings::MakeLowerCase(*iter);
    ++iter;

    char const ** end = affics + ARRAY_SIZE(affics);

    if (find(affics, end, s) == end)
      streetName += s;
  }
}

void search::GetStreetNameAsKey(string const & name, string & res)
{
  strings::SimpleTokenizer iter(name, STREET_TOKENS_SEPARATOR);
  GetStreetName(iter, res);
}

bool search::IsStreetSynonym(strings::UniString const & s)
{
  static set<strings::UniString> const kSynonyms(
      make_transform_iterator(affics, &search::NormalizeAndSimplifyString),
      make_transform_iterator(affics + ARRAY_SIZE(affics), &search::NormalizeAndSimplifyString));
  return kSynonyms.count(s) != 0;
}

bool search::ContainsNormalized(string const & str, string const & substr)
{
  strings::UniString const ustr = search::NormalizeAndSimplifyString(str);
  strings::UniString const usubstr = search::NormalizeAndSimplifyString(substr);
  return std::search(ustr.begin(), ustr.end(), usubstr.begin(), usubstr.end()) != ustr.end();
}
