#include <graphx.h>
#include <keypadc.h>
#include <math.h>
#include <stdlib.h>
#include <sys/rtc.h>
#include <sys/timers.h>
#include <ti/getcsc.h>
#include <ti/screen.h>
#include <time.h>

#define WHITE 255
#define BLACK 0
#define GREEN 37
#define RED   224
#define GOLD  228

#define HUD_H 35

#define GAME_W GFX_LCD_WIDTH
#define GAME_H (GFX_LCD_HEIGHT - HUD_H)

// UTILITY GFX FUNCTIONS
unsigned int
gfx_GetFontHeight(void)
{
	unsigned int h = gfx_SetFontHeight(1);
	gfx_SetFontHeight(h);
	return h;
}

void
gfx_PrintStringCentered(const char *str)
{
	gfx_PrintStringXY(
		str,
		(GFX_LCD_WIDTH - gfx_GetStringWidth(str)) / 2,
		(GFX_LCD_HEIGHT - gfx_GetFontHeight()) / 2
	);
}

// GAME OBJECTS

typedef struct {
	int x, y;
	int xmov, ymov;
	int col;
} Ball;

typedef struct {
	int x, y;
} Paddle;

// Ball
#define BALLTOP    (ball.y - BALL_RAD)
#define BALLBOT    (ball.y + BALL_RAD)
#define BALL_RAD   7
#define BALL_SPEED 12 // per unit time, we will move 10 units in any direction
Ball ball = {
	.x    = GAME_W / 2,
	.y    = GAME_H / 2 + HUD_H,
	.col  = RED,
	.xmov = BALL_SPEED,
	.ymov = 0 //
};

// Paddles
#define PADW   6
#define PADH   40
#define PADCOL GREEN

Paddle padd1 = {
	.x = 0,
	.y = GAME_H / 2 - PADH / 2 + HUD_H, //
};

Paddle padd2 = {
	.x = GAME_W - PADW,
	.y = GAME_H / 2 - PADH / 2 + HUD_H, //
};

// FRAME COUNTERS

char framecnt2  = 0;
char framecnt3  = 0;
char framecnt10 = 0;

// GAME VARIABLES

int          p1score, p2score;
unsigned int timeleft = 1 << 7;

void
draw(void)
{
	if (!timeleft) {
		// Time is up
		const char *str;
		if (p1score > p2score) {
			str = "PLAYER 1 WINS!";
		} else if (p2score > p1score) {
			str = "PLAYER 2 WINS!";
		} else {
			str = "DRAW...";
		}

		gfx_SetTextScale(2, 2);
		gfx_PrintStringCentered(str);
		return;
	}

	// Ball
	gfx_SetColor(ball.col);
	gfx_FillCircle(ball.x, ball.y, BALL_RAD);
	gfx_SetColor(WHITE);
	gfx_Circle(ball.x, ball.y, BALL_RAD);

	// Paddles
	gfx_SetColor(PADCOL);
	gfx_FillRectangle_NoClip(padd1.x, padd1.y, PADW, PADH);
	gfx_FillRectangle_NoClip(padd2.x, padd2.y, PADW, PADH);

	// HUD
	gfx_SetColor(WHITE);
	gfx_Line_NoClip(0, HUD_H, GFX_LCD_WIDTH, HUD_H);
	gfx_PrintStringXY("SCORE: ", 10, 10);
	gfx_PrintInt(p1score, 3);
	gfx_PrintString(" - ");
	gfx_PrintInt(p2score, 3);
	gfx_PrintStringXY("TIME LEFT: ", GFX_LCD_WIDTH - 100, 10);
	gfx_PrintUInt(timeleft, 3);

	// Update all frame counters
	framecnt2  = (framecnt2 + 1) % 2;
	framecnt3  = (framecnt3 + 1) % 3;
	framecnt10 = (framecnt10 + 1) % 10;
}

// MOVEMENT ROUTINES

int movball_lasttouch = 0;

void
movball(void)
{
	if (!framecnt10) {
		return;
	}

	// Ball going out
	if (ball.x + BALL_RAD <= 0) {
		p2score++;
		goto resetball;
	} else if (ball.x - BALL_RAD >= GAME_W) {
		p1score++;
		goto resetball;
	}

	Paddle *touchpadd = NULL;
	if (ball.x - BALL_RAD < padd1.x + PADW && BALLBOT >= padd1.y &&
	    BALLTOP <= padd1.y + PADH) {
		touchpadd = &padd1;
	} else if (ball.x + BALL_RAD > padd2.x && BALLBOT >= padd2.y && BALLTOP <= padd2.y + PADH) {
		touchpadd = &padd2;
	}

	if (touchpadd && !movball_lasttouch) {
		double theta = M_PI_4 * 0.6 *
		               (ball.y - (touchpadd->y + (float)PADH / 2)) /
		               ((float)PADH / 2);
		ball.xmov = BALL_SPEED * cos(theta) * (touchpadd == &padd2 ? -1 : 1);
		ball.ymov = BALL_SPEED * sin(theta);
		movball_lasttouch++;
	}
	if (movball_lasttouch) {
		if (movball_lasttouch++ % 30 == 0) {
			movball_lasttouch = 0;
		}
	}

	// If movespeed is odd we can stutter the ball
	ball.x += ball.xmov / 2 + (framecnt2 && ball.xmov % 2);
	ball.y += ball.ymov / 2 + (framecnt2 && ball.ymov % 2);
	// Top/bottom collision detection
	if (BALLTOP <= HUD_H) {
		ball.ymov = -ball.ymov;
		ball.y    = HUD_H + BALL_RAD;
	} else if (BALLBOT >= GFX_LCD_HEIGHT) {
		ball.ymov = -ball.ymov;
		ball.y    = GFX_LCD_HEIGHT - BALL_RAD;
	}
	
	return;

resetball:;
	ball.x    = GAME_W / 2;
	ball.y    = GAME_H / 2 + HUD_H;
	ball.xmov = BALL_SPEED * (rand() % 2 ? 1 : -1);
	ball.ymov = 0;
	padd1.y   = GAME_H / 2 - PADH / 2 + HUD_H;
	padd2.y   = GAME_H / 2 - PADH / 2 + HUD_H;
}

void
movpaddle(void)
{
	if (!framecnt10) {
		return;
	}

	if (padd1.y - 3 >= HUD_H && kb_Data[2] & kb_Ln) {
		padd1.y -= 3;
	} else if (padd1.y + PADH + 3 <= GFX_LCD_HEIGHT && kb_Data[2] & kb_Sto) {
		padd1.y += 3;
	}

	if (padd2.y - 3 >= HUD_H && kb_Data[6] & kb_Sub) {
		padd2.y -= 3;
	} else if (padd2.y + PADH + 3 <= GFX_LCD_HEIGHT && kb_Data[6] & kb_Add) {
		padd2.y += 3;
	}
}

bool twoplayer;

void
playerselect(void)
{
	gfx_ZeroScreen();

	const int dialogw = 230, dialogh = 80;
	gfx_SetColor(WHITE);
	// Draw - swap - draw - swap so we don't have to redraw rect
	gfx_Rectangle(
		(GFX_LCD_WIDTH - dialogw) / 2,
		(GFX_LCD_HEIGHT - dialogh) / 2,
		dialogw,
		dialogh
	);
	gfx_SwapDraw();
	gfx_Rectangle(
		(GFX_LCD_WIDTH - dialogw) / 2,
		(GFX_LCD_HEIGHT - dialogh) / 2,
		dialogw,
		dialogh
	);
	gfx_SwapDraw();

	for (int i = 0; i < 253; i = (i + 1) % 252) {
		kb_Scan();

		gfx_SetTextFGColor(i + 3); // teehee
		gfx_PrintStringCentered("[1] OR [2] PLAYER SELECT");
		gfx_SwapDraw();

		if (kb_Data[3] & kb_1) {
			twoplayer = false;
			break;
		} else if (kb_Data[4] & kb_2) {
			twoplayer = true;
			break;
		}

		delay(15);
	}
	gfx_SetTextFGColor(WHITE);
}

int
main(void)
{
	os_ClrHome();
	srand(time(NULL));
	rtc_Enable(RTC_SEC_INT);

	gfx_Begin();
	gfx_SetDrawBuffer();

	gfx_ZeroScreen();
	gfx_SwapDraw();

	gfx_SetTextTransparentColor(BLACK);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextBGColor(BLACK);

	playerselect();

	ball.xmov *= rand() % 2 ? 1 : -1;

	// Start on the second
	while (!rtc_ChkInterrupt(RTC_SEC_INT))
		;
	rtc_AckInterrupt(RTC_SEC_INT);

	while (!(kb_Data[1] & kb_Mode)) {
		kb_Scan();

		if (rtc_ChkInterrupt(RTC_SEC_INT) && timeleft) {
			rtc_AckInterrupt(RTC_SEC_INT);
			timeleft--;
		}

		if (timeleft) {
			movball();
			movpaddle();
		}

		gfx_ZeroScreen();
		draw();

		gfx_SwapDraw();
		kb_Scan();
	}

	gfx_End();
	return 0;
}
