//Dedicated to game listing

void printGame(u8 gameName[])
{
	u32 fullname = 0;
	
	fullname = gameName[0] << 24 | gameName[1] << 16 | gameName[2] << 8 | gameName[3];
	
	switch (fullname)
	{
		//JPN Retail
		case 0x444D504A:	//DMPJ
			draw_puts("Mario Artist Paint Studio (JPN)      ");
			break;
		case 0x444D544A:	//DMTJ
			draw_puts("Mario Artist Talent Studio (JPN)     ");
			break;
		case 0x444D424A:	//DMBJ
			draw_puts("Mario Artist Communication Kit (JPN) ");
			break;
		case 0x444D474A:	//DMGJ
			draw_puts("Mario Artist Polygon Studio (JPN)    ");
			break;
		case 0x4453434A:	//DSCJ
			draw_puts("Sim City 64 (JPN)                    ");
			break;
		case 0x4450474A:	//DPGJ
			draw_puts("Japan Pro Golf Tour 64 (JPN)         ");
			break;
		case 0x444B444A:	//DKDJ
			draw_puts("Doshin The Giant 1 (JPN)             ");
			break;
		case 0x444B494A:	//DKIJ
			draw_puts("Doshin The Giant 1 (Store Demo) (JPN)");
			break;
		case 0x4452444A:	//DRDJ
			draw_puts("Randnet Disk (JPN)                   ");
			break;
		case 0x444B4B4A:	//DKKJ
			draw_puts("Doshin The Giant Add-on (JPN)        ");
			break;
		case 0x45465A4A:	//EFZJ
			draw_puts("F-Zero X Expansion Kit (JPN)         ");
			break;
		//Unreleased
		case 0x455A4C4A:	//EZLJ
			draw_puts("Zelda Expansion Disk (JPN)           ");
			break;
		case 0x44455A41:	//DEZA
			draw_puts("Dezaemon 3D Expansion (Development)  ");
			break;
		case 0x45445A4A:	//EDZJ
			draw_puts("Dezaemon 3D Expansion (JPN)          ");
			break;
		
		//USA Retail (Potential)
		case 0x444D5045:	//DMPE
			draw_puts("Mario Artist Paint Studio (USA)      ");
			break;
		case 0x444D5445:	//DMTE
			draw_puts("Mario Artist Talent Studio (USA)     ");
			break;
		case 0x444D4245:	//DMBE
			draw_puts("Mario Artist Communication Kit (USA) ");
			break;
		case 0x444D4745:	//DMGE
			draw_puts("Mario Artist Polygon Studio (USA)    ");
			break;
		case 0x44534345:	//DSCE
			draw_puts("Sim City 64 (USA)                    ");
			break;
		case 0x44504745:	//DPGE
			draw_puts("Japan Pro Golf Tour 64 (USA)         ");
			break;
		case 0x444B4445:	//DKDE
			draw_puts("Doshin The Giant 1 (USA)             ");
			break;
		case 0x444B4945:	//DKIE
			draw_puts("Doshin The Giant 1 (Store Demo) (USA)");
			break;
		case 0x44524445:	//DRDE
			draw_puts("Randnet Disk (USA)                   ");
			break;
		case 0x444B4B45:	//DKKE
			draw_puts("Doshin The Giant Add-on (USA)        ");
			break;
		case 0x45465A45:	//EFZE
			draw_puts("F-Zero X Expansion Kit (USA)         ");
			break;
		//Unreleased
		case 0x455A4C45:	//EZLE
			draw_puts("Zelda Expansion Disk (USA)           ");
			break;
		case 0x45445A45:	//EDZE
			draw_puts("Dezaemon 3D Expansion (USA)          ");
			break;
			
		default:
			draw_puts("Unknown Disk                         ");
			break;
	}
}