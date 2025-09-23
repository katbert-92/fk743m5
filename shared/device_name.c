#include "device_name.h"
#include "platform.h"

static const char* Syllables[] = {
	// Japan
	"ka", "ki", "ku", "ke", "ko", "sa", "shi", "su", "se", "so", "ta", "chi", "tsu", "te", "to",
	"na", "ni", "nu", "ne", "no", "ha", "hi", "fu", "he", "ho", "ma", "mi", "mu", "me", "mo", "ya",
	"yu", "yo", "ra", "ri", "ru", "re", "ro", "wa", "wo", "n", "kyo", "sho", "cho", "nyo", "ryu",
	"gai", "kai", "mei", "rei", "zen", "jyo", "bai", "tei", "tou", "rin",

	// Sci-fi / Tech
	"rax", "zor", "tek", "vox", "cy", "kex", "zon", "xan", "nex", "qu", "ly", "tri", "zen", "dra",
	"vex", "tor", "hex", "cru", "zar", "myr", "syn", "neo", "psy", "zir", "ona", "ixa", "kor",
	"oma", "kai", "eli", "vyn", "ora", "aer", "ion", "vek", "sky", "pho", "lux", "aon", "elix",
	"zy", "cry", "arc", "nov", "lun", "sol", "mek", "thr", "dax", "xil", "yor", "va", "zeo", "ume",
	"kai", "sai", "vra", "exa", "xen", "qor", "ren", "jyn", "noq", "kyu", "zo", "xe", "ju", "qa",
	"byo", "sei", "rin", "ryo"};

static char DeviceName[16];

void DeviceName_Generate(void) {
	u32* pUidPtr = Pl_UID_GetStrAndPtr(NULL);

	u32 hash = 0;
	for (u32 i = 0; i < 3; i++) {
		u32 val = pUidPtr[i];
		hash	= (hash << 5) ^ val ^ (hash >> 2);
	}

	const char* s1 = Syllables[hash % NUM_ELEMENTS(Syllables)];
	const char* s2 = Syllables[(hash / NUM_ELEMENTS(Syllables)) % NUM_ELEMENTS(Syllables)];
	u32 digit	   = hash % 10;

	snprintf(DeviceName, NUM_ELEMENTS(DeviceName), "%s%d%s", s1, digit, s2);
}

const char* DeviceName_GetPtr(void) {
	return (const char*)DeviceName;
}
