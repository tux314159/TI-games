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
#define BALLTOP   (ball.y - BALL_RAD)
#define BALLBOT   (ball.y + BALL_RAD)
#define BALL_XSPD 4
#define BALL_RAD  7
Ball ball = {
	.x    = GAME_W / 2,
	.y    = GAME_H / 2 + HUD_H,
	.col  = RED,
	.xmov = BALL_XSPD,
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

int p1score, p2score;
int timeleft = 1 << 7;

void
draw(void)
{
	if (timeleft < 1) {
		// Time is up
		gfx_SetTextScale(2, 2);
		if (p1score > p2score) {
			gfx_PrintStringXY("PLAYER 1 WINS!", 50, 100);
		} else if (p2score > p1score) {
			gfx_PrintStringXY("PLAYER 2 WINS!", 50, 100);
		} else {
			gfx_PrintStringXY("DRAW", 50, 50);
		}
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
	gfx_PrintInt(timeleft, 3);

	if (rtc_ChkInterrupt(RTC_SEC_INT)) {
		rtc_AckInterrupt(RTC_SEC_INT);
		timeleft--;
	}

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

	ball.x += ball.xmov;
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
		ball.xmov = -ball.xmov;
		float padportion =
			(ball.y - (touchpadd->y + (float)PADH / 2)) / ((float)PADH / 2);
		ball.ymov = 8 * padportion;
		movball_lasttouch++;
	}
	if (movball_lasttouch) {
		if (movball_lasttouch++ % 30 == 0) {
			movball_lasttouch = 0;
		}
	}

	if (!framecnt2) {
		ball.y += ball.ymov;
		// Top/bottom collision detection
		if (BALLTOP <= HUD_H) {
			ball.ymov = -ball.ymov;
			ball.y    = HUD_H + BALL_RAD;
		} else if (BALLBOT >= GFX_LCD_HEIGHT) {
			ball.ymov = -ball.ymov;
			ball.y    = GFX_LCD_HEIGHT - BALL_RAD;
		}
	}

	return;

resetball:;
	ball.x    = GAME_W / 2;
	ball.y    = GAME_H / 2 + HUD_H;
	ball.xmov = BALL_XSPD * (rand() % 2 ? 1 : -1);
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
	gfx_SetTextConfig(gfx_text_clip);
	ball.xmov *= rand() % 2 ? 1 : -1;

	// Start on the second
	while (!rtc_ChkInterrupt(RTC_SEC_INT))
		;
	rtc_AckInterrupt(RTC_SEC_INT);

	while (!(kb_Data[1] & kb_Mode)) {
		kb_Scan();

		movball();
		movpaddle();

		gfx_ZeroScreen();
		draw();

		gfx_SwapDraw();
		kb_Scan();
	}

	gfx_End();
	return 0;
}
