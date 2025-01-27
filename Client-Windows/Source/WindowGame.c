#include "window-game.h"

typedef struct chess_color_rgb {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} ChessColorRGB;

typedef enum sprites {
	SPRITE_MOVE,
	SPRITE_CAPTURE,
	SPRITE_CHECK,
	SPRITE_HOVER,
	SPRITE_OVERLAY,
	SPRITE_PROMOTION,
	SPRITE_COUNT
} Sprites;

typedef enum buffer_type {
	BUFFER_SPRITES,
	BUFFER_COMP,
	BUFFER_BOARD,
	BUFFER_LABELS,
	BUFFER_PIECE_BG,
	BUFFER_PIECES,
	BUFFER_HOVER,
	BUFFER_CURSOR,
	BUFFER_COUNT
} BufferType;

typedef struct buffers {
	HDC		hMemDC;
	HBITMAP hBuffer;
	int		width;
	int		height;
	void*	pvBits;
} Buffers[BUFFER_COUNT];

typedef enum action_flags {
	NO_ACTION_FLAGS = 0,
	IS_SELECTED		= 1,
	IS_DRAGGING		= 1 << 1,
} ActionFlags;

/*
* 01 - 04 ActionFlags
* 05 - 12 sourceX
* 13 - 20 sourceY
* 21 - 28 sourceNr
* 29 - 32 empty
*/
typedef uint32_t ActionState;

#define ACTION_SELECTED(action)      ((action) & 0x0001)
#define ACTION_DRAGGING(action)      ((action >> 1)  & 0x0001)
#define ACTION_SOURCE_X(action)      ((action >> 4)  & 0x00FF)
#define ACTION_SOURCE_Y(action)      ((action >> 12) & 0x00FF)
#define ACTION_SOURCE_NR(action)     ((action >> 20) & 0x00FF)
#define SET_ACTION_SOURCE_X(source)  ((source) << 4)
#define SET_ACTION_SOURCE_Y(source)  ((source) << 12)
#define SET_ACTION_SOURCE_NR(source) ((source) << 20)

typedef enum RenderFlags {
	RENDER_WINDOW	   = 1,
	RENDER_BACKBUFFER  = 1 << 1,
	RENDER_PIECESB     = 1 << 2,
	RENDER_HIGHLIGHTSB = 1 << 3,
	RENDER_HOVER	   = 1 << 4,
	RENDER_PROMOTION   = 1 << 5,
};

//typedef enum update_flags {
//
//};

/*
* 01 - 08 render flags
* 09 - 16 update flags
* 17 - 32 empty
*/
typedef uint32_t RenderState;

typedef uint64_t BitBoard64;

#define CHECK_BIT(bits, pos) (bits & (1ULL << pos))

typedef enum mouse_input {
	MOUSE_LEFT_CLICK,
	MOUSE_LEFT_RELEASE,
	MOUSE_RIGHT_CLICK,
	MOUSE_RIGHT_RELEASE,
	MOUSE_MOVE,
} MouseInput;

static UINT g_allowWindowRendering = 1;
static UINT g_renderPiecesBuffer = 0;
static UINT g_renderHover = 0;
static UINT g_renderHoverBuffer = 0;
static bool g_pieceAlreadySelected = false;

static UINT g_promotionMode = 0;
static UINT g_promotionInitialized = 0;
static Move g_promotionMove = 0;

static Buffers     g_buffers;
static BYTE		   g_buffersInitialized[BUFFER_COUNT] = { 0 };
static BitBoard64  g_hoverBitBoard = 0;
static ActionState g_ActionState = 0;

static HBRUSH g_hBrushChessWhite = NULL;
static HBRUSH g_hBrushChessBlack = NULL;

static ChessColorRGB g_ChessRGBw = {
	.red = 240,
	.green = 211,
	.blue = 186
};

static ChessColorRGB g_ChessRGBb = {
	.red = 119,
	.green = 89,
	.blue = 76
};

static const ChessColorRGB g_sqHoverColor = {
	.red = 20,
	.green = 85,
	.blue = 30,
};

static void InitializeBuffer(BufferType type, HDC hDC);

static RECT GetGameWindowRect() {
	RECT rect;
	HWND hWnd = FindWindow(g_szGameWndClass, g_szGameWndTitle);

	GetClientRect(hWnd, &rect);
	return rect;
}

static void PremultiplyAlpha(HBITMAP bitmap) {
	BITMAP bmp;
	GetObject(bitmap, sizeof(BITMAP), &bmp);
	DWORD* pixel = (DWORD*)bmp.bmBits;
	int pixelCount = bmp.bmWidth * bmp.bmHeight;

	for (int i = 0; i < pixelCount; i++) {
		DWORD color = pixel[i];
		BYTE b = color & 0xFF;
		BYTE g = (color >> 8) & 0xFF;
		BYTE r = (color >> 16) & 0xFF;
		BYTE a = color >> 24;

		b = (b * a) / 255;
		g = (g * a) / 255;
		r = (r * a) / 255;

		pixel[i] = (a << 24) | (r << 16) | (g << 8) | b;
	}
}

static void SetAlphaSpritesBuffer(RECT* rect, BYTE alpha) {
	DWORD* pixel = (DWORD*)g_buffers[BUFFER_SPRITES].pvBits;
	if (!pixel) {
		return;
	}

	int bufferWidth = g_buffers[BUFFER_SPRITES].width;
	int bufferHeight = g_buffers[BUFFER_SPRITES].height;

	if (rect->left < 0 || rect->right > bufferWidth || rect->top < 0 || rect->bottom > bufferHeight) {
		return;
	}
	int width = rect->right - rect->left;
	int height = rect->bottom - rect->top;

	int pixelCount = width * height;

	int currPixel = bufferWidth * rect->top + rect->left;
	int position = 0;
	for (int i = 0; i < pixelCount; i++) {
		if (position >= width) {
			currPixel += bufferWidth - width;
			position = 0;
		}

		DWORD color = pixel[currPixel];
		BYTE b = color & 0xFF;
		BYTE g = (color >> 8) & 0xFF;
		BYTE r = (color >> 16) & 0xFF;
		BYTE a = color >> 24;
		if ((b + g + r) > 0) {
			a = alpha;
		}

		pixel[currPixel++] = (a << 24) | (r << 16) | (g << 8) | b;
		position++;
	}
}

static void CopySpriteToBuffer(HDC hBuffer, Sprites sprite, int column, int row) {
	RECT rect = {
		.left = column * g_fieldSize,
		.top = row * g_fieldSize,
		.right = column * g_fieldSize + g_fieldSize,
		.bottom = row * g_fieldSize + g_fieldSize
	};

	BLENDFUNCTION blendFunction;
	blendFunction.BlendOp = AC_SRC_OVER;
	blendFunction.AlphaFormat = AC_SRC_ALPHA;
	blendFunction.SourceConstantAlpha = 255;
	blendFunction.BlendFlags = 0;

	AlphaBlend(
		hBuffer, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		g_buffers[BUFFER_SPRITES].hMemDC, sprite * g_fieldSize, 0, g_fieldSize, g_fieldSize,
		blendFunction
	);
}

static HDC CreateBuffer(BufferType type, HDC hDC, int width, int height) {
	HDC		   hMemDC;
	BITMAPINFO bmi;
	HBITMAP	   hBitmap;
	void* pvBits;

	hMemDC = g_buffers[type].hMemDC;
	if (hMemDC) {
		return hMemDC;
	}

	g_buffers[type].hMemDC = hMemDC = CreateCompatibleDC(hDC);

	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
	g_buffers[type].hBuffer = hBitmap;
	g_buffers[type].pvBits = pvBits;

	SelectObject(hMemDC, g_buffers[type].hBuffer);

	g_buffers[type].width = width;
	g_buffers[type].height = height;

	if (!g_buffersInitialized[type]) {
		g_buffersInitialized[type] = 1;
		InitializeBuffer(type, hMemDC);
	}

	return hMemDC;
}

static HDC GetBuffer(BufferType type, HDC hDC, int width, int height) {
	return CreateBuffer(type, hDC, width, height);
}

static HDC GetBackBuffer(HDC hDC) {
	return GetBuffer(BUFFER_COMP, hDC, GAMEWND_WIDTH, GAMEWND_HEIGHT);
}

static HDC GetBoardBuffer(HDC hDC) {
	return GetBuffer(BUFFER_BOARD, hDC, GAMEWND_WIDTH, GAMEWND_HEIGHT);
}

static HDC GetLabelsBuffer(HDC hDC) {
	return GetBuffer(BUFFER_LABELS, hDC, GAMEWND_WIDTH, GAMEWND_HEIGHT);
}

static HDC GetPieceBgBuffer(HDC hDC) {
	return GetBuffer(BUFFER_PIECE_BG, hDC, GAMEWND_WIDTH, GAMEWND_HEIGHT);
}

static HDC GetPiecesBuffer(HDC hDC) {
	return GetBuffer(BUFFER_PIECES, hDC, GAMEWND_WIDTH, GAMEWND_HEIGHT);
}

static HDC GetCursorBuffer(HDC hDC) {
	return GetBuffer(BUFFER_CURSOR, hDC, GAMEWND_WIDTH, GAMEWND_HEIGHT);
}

static HDC GetHoverBuffer(HDC hDC) {
	return GetBuffer(BUFFER_HOVER, hDC, GAMEWND_WIDTH, GAMEWND_HEIGHT);
}

static HDC GetSpritesBuffer(HDC hDC) {
	return GetBuffer(BUFFER_SPRITES, hDC, g_fieldSize * ((6 * 2) + SPRITE_COUNT), g_fieldSize);
}

static HDC GetBufferByType(HDC hDC, BufferType buffer) {
	switch (buffer) {
	case BUFFER_SPRITES: {
		return GetSpritesBuffer(hDC);
	}
	case BUFFER_COMP: {
		return GetBackBuffer(hDC);
	}
	case BUFFER_BOARD: {
		return GetBoardBuffer(hDC);
	}
	case BUFFER_LABELS: {
		return GetLabelsBuffer(hDC);
	}
	case BUFFER_PIECE_BG: {
		return GetPieceBgBuffer(hDC);
	}
	case BUFFER_PIECES: {
		return GetPiecesBuffer(hDC);
	}
	case BUFFER_HOVER: {
		return GetHoverBuffer(hDC);
	}
	case BUFFER_CURSOR: {
		return GetCursorBuffer(hDC);
	}
	}
	return NULL;
}

static void RenderCheck(HDC hDC, Board* board, GameFlags flags) {
	ChessColor kingColor  = GAME_FLAG_WHITE_MOVES(flags) ? WHITE : BLACK;
	Square*    kingSquare = findKing(board, kingColor);

	BYTE column = kingSquare->nr % BOARD_COLS;
	BYTE row    = BOARD_ROWS - 1 - kingSquare->nr / BOARD_ROWS;

	CopySpriteToBuffer(hDC, SPRITE_CHECK, column, row);
}

static HBRUSH SelectChessBrush(BYTE x, BYTE y) {
	if (!g_hBrushChessWhite) {
		g_hBrushChessWhite = CreateSolidBrush(
			RGB(g_ChessRGBw.red, g_ChessRGBw.green, g_ChessRGBw.blue)
		);
	}

	if (!g_hBrushChessBlack) {
		g_hBrushChessBlack = CreateSolidBrush(
			RGB(g_ChessRGBb.red, g_ChessRGBb.green, g_ChessRGBb.blue)
		);
	}

	return ((x + y) % 2) ? g_hBrushChessBlack : g_hBrushChessWhite;
}

static void FillBoardRect(HDC hDC, BYTE x, BYTE y) {
	RECT rect = {
		x * g_fieldSize, y * g_fieldSize,
		(x + 1) * g_fieldSize, (y + 1) * g_fieldSize
	};
	FillRect(hDC, &rect, SelectChessBrush(x, y));
}

static int DrawSVGPiece(HDC hDC, Piece piece, int x, int y) {
	cairo_surface_t		*cairo_surface;
	cairo_t				*cairo;
	RsvgHandle			*rsvg_handle;
	RsvgDimensionData	dimensions;
	GError				*gError = NULL;
	char				file_path[FILEPATH_SIZE];
	char				*fileName;

	strcpy(file_path, "../Chess-Core/");
	strcat(file_path, g_piecesPath);
	fileName = getFilenameFromPiece(piece);
	strcat(file_path, fileName);

	rsvg_handle = rsvg_handle_new_from_file(file_path, &gError);

	if (gError != NULL) {
		MessageBoxW(NULL, (LPCWSTR)gError->message, L"Error loading SVG", MB_OK);
		g_error_free(gError);
		return 1;
	}

	rsvg_handle_get_dimensions(rsvg_handle, &dimensions);

	cairo_surface = cairo_win32_surface_create(hDC);
	cairo = cairo_create(cairo_surface);

	int padding = 10;
	int posX = x * g_fieldSize + padding / 2;
	int posY = y * g_fieldSize + padding / 2;

	cairo_translate(cairo, posX, posY);
	cairo_scale(
		cairo,
		((float)(g_fieldSize - padding)) / dimensions.width,
		((float)(g_fieldSize - padding)) / dimensions.height
	);

	rsvg_handle_render_cairo(rsvg_handle, cairo);

	cairo_destroy(cairo);
	cairo_surface_destroy(cairo_surface);
	g_object_unref(rsvg_handle);

	return 0;
}

static void RenderPieces(HDC hDC, Board* board) {
	uint8_t x, y;
	Piece piece;
	for (x = 0; x < BOARD_ROWS; x++) {
		for (y = 0; y < BOARD_COLS; y++) {
			if (board->squares[x][y].piece.type == NO_PIECE) {
				continue;
			}

			piece = board->squares[x][y].piece;
			CopySpriteToBuffer(
				hDC, 
				SPRITE_COUNT + piece.type - 1 + (piece.color == WHITE ? 0 : 6), 
				y, x
			);
		}
	}
}

static void RenderTileNumbers(cairo_t* cairo) {
	ChessColorRGB	rgb;
	char			text[4];
	uint8_t			col, row, i;

	for (i = 0; i < BOARD_ROWS * BOARD_COLS; i++) {
		snprintf(text, sizeof(text), "%d", i);

		col = i % BOARD_COLS;
		row = BOARD_ROWS - 1 - i / BOARD_ROWS;

		rgb = (row + col) % 2 == 0 ? g_ChessRGBb : g_ChessRGBw;

		cairo_set_source_rgba(
			cairo,
			rgb.red / 255.0, rgb.green / 255.0, rgb.blue / 255.0, 1
		);

		cairo_move_to(cairo, col * g_fieldSize + 5, row * g_fieldSize + 22);
	
		cairo_show_text(cairo, text);
	}
}

static void RenderRowNumbers(cairo_t* cairo) {
	ChessColorRGB	rgb;
	char			text[4];
	uint8_t			rowNumber, i;

	rowNumber = BOARD_ROWS;

	for (i = 0; i < BOARD_ROWS; i++) {
		snprintf(text, sizeof(text), "%d", rowNumber);

		rgb = i % 2 == 0 ? g_ChessRGBw : g_ChessRGBb;
		cairo_set_source_rgba(
			cairo,
			rgb.red / 255.0, rgb.green / 255.0, rgb.blue / 255.0, 1
		);

		cairo_move_to(cairo, BOARD_COLS * g_fieldSize - 15, i * g_fieldSize + 22);
		cairo_show_text(cairo, text);

		rowNumber--;
	}
}

static void RenderColumnLetters(cairo_t* cairo) {
	ChessColorRGB rgb;
	char		  text[4];
	uint8_t		  i;

	for (i = 0; i < BOARD_COLS; i++) {
		snprintf(text, sizeof(text), "%c", i + 'a');

		rgb = i % 2 == 0 ? g_ChessRGBw : g_ChessRGBb;
		cairo_set_source_rgba(
			cairo,
			rgb.red / 255.0, rgb.green / 255.0, rgb.blue / 255.0, 1
		);

		cairo_move_to(cairo, i * g_fieldSize + 5, BOARD_ROWS * g_fieldSize - 5);
		cairo_show_text(cairo, text);
	}
}

static void RenderLabels(HDC hDC) {
	void			*pvBits;
	int				stride;
	cairo_surface_t	*cairo_surface;
	cairo_t			*cairo;

	pvBits = g_buffers[BUFFER_LABELS].pvBits;

	stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, GAMEWND_WIDTH);
	cairo_surface = cairo_image_surface_create_for_data(
		pvBits, CAIRO_FORMAT_ARGB32, GAMEWND_WIDTH, GAMEWND_HEIGHT, stride
	);
	cairo = cairo_create(cairo_surface);
	
	cairo_select_font_face(
		cairo, "Arial",
		CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD
	);
	cairo_set_font_size(cairo, 20);
	
	RenderColumnLetters(cairo);
	RenderRowNumbers(cairo);

#ifdef DEBUG
	RenderTileNumbers(cairo);
#endif // !DEBUG

	PremultiplyAlpha(g_buffers[BUFFER_LABELS].hBuffer);

	cairo_destroy(cairo);
	cairo_surface_destroy(cairo_surface);
}

static void RenderBoard(HDC hDC) {
	BYTE x, y;
	for (y = 0; y < BOARD_COLS; y++) {
		for (x = 0; x < BOARD_ROWS; x++) {
			FillBoardRect(hDC, y, x);
		}
	}
}

static void DrawRectOnSquare(HDC hDC, BYTE column, BYTE row, BYTE alpha) {
	RECT rect = {
		.left	= column * g_fieldSize,
		.top	= row	 * g_fieldSize,
		.right	= column * g_fieldSize + g_fieldSize,
		.bottom = row	 * g_fieldSize + g_fieldSize
	};

	Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);

	if (alpha) {
		SetAlphaSpritesBuffer(&rect, alpha);
	}
}

static void DrawCircleOnSquare(HDC hDC, BYTE column, BYTE row, BYTE alpha) {
	RECT rect = {
		.left	= column * g_fieldSize + g_fieldSize / 3,
		.top	= row	 * g_fieldSize + g_fieldSize / 3,
		.right	= column * g_fieldSize - g_fieldSize / 3 + g_fieldSize,
		.bottom = row	 * g_fieldSize - g_fieldSize / 3 + g_fieldSize
	};

	Ellipse(hDC, rect.left, rect.top, rect.right, rect.bottom);

	if (alpha) {
		SetAlphaSpritesBuffer(&rect, alpha);
	}
}

static void DrawCaptureHighlight(HDC hDC, BYTE column, BYTE row, COLORREF color, BYTE alpha) {
	DrawRectOnSquare(hDC, column, row, alpha);

	HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
	HBRUSH hOldBrush = SelectObject(hDC, hBrush);
	SetDCPenColor(hDC, RGB(0, 0, 0));

	RECT rect = {
		.left	= column * g_fieldSize - g_fieldSize / 15,
		.top	= row	 * g_fieldSize - g_fieldSize / 15,
		.right	= column * g_fieldSize + g_fieldSize + g_fieldSize / 15,
		.bottom = row	 * g_fieldSize + g_fieldSize + g_fieldSize / 15
	};

	Ellipse(hDC, rect.left, rect.top, rect.right, rect.bottom);

	SetDCPenColor(hDC, color);
	SelectObject(hDC, hOldBrush);
	DeleteObject(hBrush);
}

static DWORD CalculateGradientColor(COLORREF color1, COLORREF color2, float percent) {
	BYTE a = min(255, 255 + percent * (0 - 255));
	BYTE r = min(255, GetRValue(color1) + percent * (GetRValue(color2) - GetRValue(color1)));
	BYTE g = min(255, GetGValue(color1) + percent * (GetGValue(color2) - GetGValue(color1)));
	BYTE b = min(255, GetBValue(color1) + percent * (GetBValue(color2) - GetBValue(color1)));

	return ((a << 24) | (r << 16) | (g << 8) | b);
}

static void DrawGradientCircleOnSquare(struct buffers* buffer, BYTE column, BYTE row, COLORREF color1, COLORREF color2) {
	RECT rect = {
		.left	= column * g_fieldSize, 
		.top	= row	 * g_fieldSize, 
		.right	= column * g_fieldSize + g_fieldSize, 
		.bottom = row	 * g_fieldSize + g_fieldSize 
	};

	int reWidth  = rect.right - rect.left;
	int reHeight = rect.bottom - rect.top;
	int ciWidth  = reWidth - g_fieldSize / 15;
	int ciHeight = reHeight - g_fieldSize / 15;
	int centerX  = (rect.left + rect.right) / 2;
	int centerY  = (rect.top + rect.bottom) / 2;
	
	int radius = min(ciWidth, ciHeight) / 2;
	int radiusSqr = radius * radius;

	DWORD* pixel = buffer->pvBits;
	float posSqr, pos, percent;
	for (int y = centerY - radius; y <= centerY + radius; y++) {
		for (int x = centerX - radius; x <= centerX + radius; x++) {
			posSqr = (x - centerX) * (x - centerX) + (y - centerY) * (y - centerY);
			if (posSqr > radiusSqr) {
				continue;
			}
			pos = sqrt(posSqr);
			percent = pos == 0 ? 0 : pos / radius;
			pixel[y * buffer->width + x] = CalculateGradientColor(color1, color2, percent);
		}
	}
}

static void InitializeSpritesBuffer(HDC hDC) {
	// Sprites      = 0 - SPRITE_COUNT
	// White Pieces = SPRITE_COUNT + PieceType
	// Black Pieces = SPRITE_COUNT + PieceType + 6
	HDC hSpritesBuffer = GetSpritesBuffer(hDC);

	COLORREF highlightColor = RGB(g_sqHoverColor.red, g_sqHoverColor.green, g_sqHoverColor.blue);
	COLORREF checkColor		= RGB(255,0,0);
	COLORREF promotionColor = RGB(209, 134, 00);
	COLORREF whiteColor		= RGB(255, 255, 255);
	COLORREF blackColor		= RGB(0, 0, 1);
	
	HBRUSH hBrush = CreateSolidBrush(highlightColor);
	HBRUSH hOldBrush = SelectObject(hSpritesBuffer, hBrush);

	SetDCPenColor(hSpritesBuffer, highlightColor);
	SelectObject(hSpritesBuffer, GetStockObject(DC_PEN));

	BYTE alpha = 127;
	DrawRectOnSquare(hSpritesBuffer, SPRITE_HOVER, 0, alpha);
	DrawCircleOnSquare(hSpritesBuffer, SPRITE_MOVE, 0, alpha);
	DrawCaptureHighlight(hSpritesBuffer, SPRITE_CAPTURE, 0, highlightColor, alpha);
	DrawGradientCircleOnSquare(&g_buffers[BUFFER_SPRITES], SPRITE_CHECK, 0, checkColor, blackColor);
	DrawGradientCircleOnSquare(&g_buffers[BUFFER_SPRITES], SPRITE_PROMOTION, 0, promotionColor, blackColor);

	HBRUSH hOverlayBrush = CreateSolidBrush(blackColor);
	HBRUSH hOldOverlayBrush = SelectObject(hSpritesBuffer, hOverlayBrush);
	
	SetDCPenColor(hSpritesBuffer, blackColor);
	SelectObject(hSpritesBuffer, GetStockObject(DC_PEN));

	DrawRectOnSquare(hSpritesBuffer, SPRITE_OVERLAY, 0, alpha);

	SelectObject(hSpritesBuffer, hOldOverlayBrush);
	DeleteObject(hOverlayBrush);

	Piece piece = initializeEmptyPiece();
	BYTE type, color, isWhite;
	for (int i = 0; i < 6 * 2; i++) {
		isWhite = i < 6;
		type = isWhite ? i + 1 : i - 5;
		color = isWhite ? WHITE : BLACK;
		piece.type = type;
		piece.color = color;
		DrawSVGPiece(hSpritesBuffer, piece, i + SPRITE_COUNT, 0);
	}

	SelectObject(hSpritesBuffer, hOldBrush);
	DeleteObject(hBrush);
}

static void InitializeBuffer(BufferType type, HDC hDC) {
	switch (type) {
	case BUFFER_SPRITES: {
		InitializeSpritesBuffer(hDC);
		break;
	}
	case BUFFER_BOARD: {
		RenderBoard(hDC);
		break;
	}
	case BUFFER_LABELS: {
		RenderLabels(hDC);
		break;
	}
	case BUFFER_PIECES: {
		GameInstance* game = getGameInstance();
		RenderPieces(hDC, &game->state.board);
		break;
	}
	case BUFFER_PIECE_BG: {
		GameInstance* game = getGameInstance();
		if (GAME_FLAG_CHECK(game->state.flags)) {
			RenderCheck(hDC, &game->state.board, game->state.flags);
		}
		break;
	}
	default:
		break;
	}
}

static void ClearGameWnd(HWND hWnd) {
	RECT rect;

	GetClientRect(hWnd, &rect);
	InvalidateRect(hWnd, &rect, 1);

	g_allowWindowRendering = 1;
}

static void ClearBufferByType(HWND hWnd, BufferType type) {
	HDC hDC = GetDC(hWnd);
	HDC hBuffer = GetBufferByType(hDC, type);
	
	ReleaseDC(hWnd, hDC);

	BitBlt(
		hBuffer,
		0, 0,
		g_buffers[type].width, g_buffers[type].height,
		NULL,
		0, 0,
		BLACKNESS
	);
}

static void ComposeBuffers(HDC hDC) {
	HDC hBackBuffer		  = GetBackBuffer(hDC);
	HDC hSpritesBuffer	  = GetSpritesBuffer(hDC);
	HDC hBoardBuffer	  = GetBoardBuffer(hDC);
	HDC hLabelsBuffer	  = GetLabelsBuffer(hDC);
	HDC hPieceBgBuffer	  = GetPieceBgBuffer(hDC);
	HDC hPiecesBuffer	  = GetPiecesBuffer(hDC);
	HDC hHoverBuffer	  = GetHoverBuffer(hDC);
	HDC hCursorBuffer	  = GetCursorBuffer(hDC);

	BitBlt(hBackBuffer, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT, hBoardBuffer, 0, 0, SRCCOPY);
	
	BLENDFUNCTION blendFunction;
	blendFunction.BlendOp = AC_SRC_OVER;
	blendFunction.AlphaFormat = AC_SRC_ALPHA;
	blendFunction.SourceConstantAlpha = 255;
	blendFunction.BlendFlags = 0;

	if (ACTION_SELECTED(g_ActionState) == 1 || g_renderHoverBuffer) {
		AlphaBlend(
			hBackBuffer, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT,
			hHoverBuffer, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT,
			blendFunction
		);
	}

	AlphaBlend(
		hBackBuffer, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT,
		hPieceBgBuffer, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT,
		blendFunction
	);

	AlphaBlend(
		hBackBuffer, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT,
		hPiecesBuffer, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT,
		blendFunction
	);

	if (g_renderHover) {
		BYTE squareNr = log2(g_hoverBitBoard);
		BYTE column = squareNr % 8;
		BYTE row = BOARD_ROWS - 1 - squareNr / BOARD_ROWS;

		RECT rect = {
			.left	= column * g_fieldSize,
			.top	= row    * g_fieldSize,
			.right	= column * g_fieldSize + g_fieldSize,
			.bottom = row    * g_fieldSize + g_fieldSize,
		};

		AlphaBlend(
			hBackBuffer, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
			hSpritesBuffer, SPRITE_HOVER * g_fieldSize, 0, g_fieldSize, g_fieldSize,
			blendFunction
		);
	}

	AlphaBlend(
		hBackBuffer, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT,
		hLabelsBuffer, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT,
		blendFunction
	);

	AlphaBlend(
		hBackBuffer, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT,
		hCursorBuffer, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT,
		blendFunction
	);
}

static void CopyPieceToCursorBuffer(HWND hWnd, int column, int row) {
	HDC hDC = GetDC(hWnd);
	HDC hPiecesBuffer = GetPiecesBuffer(hDC);
	HDC hCursorBuffer = GetCursorBuffer(hDC);

	ReleaseDC(hWnd, hDC);

	BitBlt(hCursorBuffer, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT, NULL, 0, 0, BLACKNESS);

	BitBlt(
		hCursorBuffer,
		column - (g_fieldSize / 2),
		row - (g_fieldSize / 2),
		g_fieldSize, g_fieldSize,
		hPiecesBuffer,
		(ACTION_SOURCE_X(g_ActionState) * g_fieldSize),
		(ACTION_SOURCE_Y(g_ActionState) * g_fieldSize),
		SRCCOPY
	);
}

static void DrawPossibleMoves(HWND hWnd, MoveArray* moveTable, BYTE squareNr) {
	HDC hDC = GetDC(hWnd);
	HDC hHoverBuffer = GetHoverBuffer(hDC);
	ClearBufferByType(hWnd, BUFFER_HOVER);
	ReleaseDC(hWnd, hDC);
	
	MoveArray* moves = &moveTable[squareNr];
	
	Move move;
	MoveFlags flags;
	BYTE column, row;
	for (int i = 0; i < moves->size; i++) {
		move   = moves->moves[i];
		flags  = MOVE_FLAGS(move);
		column = MOVE_TARGET(move) % BOARD_COLS;
		row    = BOARD_ROWS - 1 - MOVE_TARGET(move) / BOARD_ROWS;

		if (MOVE_FLAG_CHECK(flags) == 1) {
			continue;
		}

		if (MOVE_FLAG_CAPTURE(flags) == 1
		 || MOVE_FLAG_CASTLING(flags) == 1)
		{
			CopySpriteToBuffer(hHoverBuffer, SPRITE_CAPTURE, column, row);
			continue;
		}

		CopySpriteToBuffer(hHoverBuffer, SPRITE_MOVE, column, row);
	}
	column = squareNr % 8;
	row = BOARD_ROWS - 1 - squareNr / BOARD_ROWS;
	CopySpriteToBuffer(hHoverBuffer, SPRITE_HOVER, column, row);
}

static void SetInitActionState(BYTE column, BYTE row, ChessColor color) {
	g_ActionState  = IS_SELECTED | IS_DRAGGING;
	g_ActionState |= SET_ACTION_SOURCE_X(column);
	g_ActionState |= SET_ACTION_SOURCE_Y(row);
	g_ActionState |= SET_ACTION_SOURCE_NR((BOARD_ROWS - row) * BOARD_ROWS - BOARD_COLS + column);
}

static void DrawPromotionBackground(HDC hDC, HDC hSpritesBuffer) {
	BLENDFUNCTION blendFunction;
	blendFunction.BlendOp = AC_SRC_OVER;
	blendFunction.AlphaFormat = AC_SRC_ALPHA;
	blendFunction.SourceConstantAlpha = 255;
	blendFunction.BlendFlags = 0;

	AlphaBlend(
		hDC, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT,
		hSpritesBuffer, SPRITE_OVERLAY * g_fieldSize, 0, g_fieldSize, g_fieldSize,
		blendFunction
	);
}

static void DrawPromotionPieces(HDC hDC, HDC hSpritesBuffer, Move move) {
	BYTE target = MOVE_TARGET(move);
	BYTE column = target % BOARD_COLS;
	BYTE row = BOARD_ROWS - 1 - target / BOARD_ROWS;

	ChessColor color = row == 0 ? WHITE : BLACK;

	COLORREF highlightColor = RGB(200, 200, 200);

	HBRUSH hBrush = CreateSolidBrush(highlightColor);
	HBRUSH hOldBrush = SelectObject(hDC, hBrush);

	SetDCPenColor(hDC, highlightColor);
	SelectObject(hDC, GetStockObject(DC_PEN));

	Piece piece;
	piece.color = color;
	for (int i = 0; i < 4; i++) {
		piece.type = 2 + i;
		DrawRectOnSquare(hDC, column, row, 0);
		CopySpriteToBuffer(hDC, SPRITE_PROMOTION, column, row);
		CopySpriteToBuffer(hDC, SPRITE_COUNT + piece.type - 1 + (piece.color == WHITE ? 0 : 6), column, row);
		row = color == WHITE ? row + 1 : row - 1;
	}

	SelectObject(hDC, hOldBrush);
	DeleteObject(hBrush);
}

static void DrawPromotionSelection(HDC hDC, Move move) {
	ComposeBuffers(hDC);

	HDC hBackBuffer = GetBackBuffer(hDC);
	HDC hSpritesBuffer = GetSpritesBuffer(hDC);

	BYTE source = MOVE_SOURCE(move);
	BYTE column = source % BOARD_COLS;
	BYTE row = BOARD_ROWS - 1 - source / BOARD_ROWS;

	BitBlt(hBackBuffer,
		column * g_fieldSize,
		row * g_fieldSize,
		g_fieldSize,
		g_fieldSize,
		hSpritesBuffer,
		SPRITE_HOVER * g_fieldSize,
		0,
		SRCCOPY
	);

	DrawPromotionBackground(hBackBuffer, hSpritesBuffer);
	DrawPromotionPieces(hBackBuffer, hSpritesBuffer, move);

	BitBlt(
		hDC,
		0, 0,
		GAMEWND_WIDTH,
		GAMEWND_HEIGHT,
		hBackBuffer,
		0, 0,
		SRCCOPY
	);
}

static void InitializePromotionMode(HWND hWnd, Move move) {
	g_promotionMode = 1;
	g_promotionMove = move;
	g_hoverBitBoard = 0;
	g_renderHover = 0;

	ClearBufferByType(hWnd, BUFFER_HOVER);

	HDC hDC = GetDC(hWnd);
	
	DrawPromotionSelection(hDC, move);
	g_allowWindowRendering = 1;

	ReleaseDC(hWnd, hDC);
}

static void MoveDraggedPiece(HWND hWnd, GameInstance* game, Square* targetSquare, Move move) {
	uint8_t flags = MOVE_FLAGS(move);

	if (MOVE_FLAG_CHECK(flags)) {
		return;
	}

	if (MOVE_FLAG_PROMOTION(flags)) {
		InitializePromotionMode(hWnd, move);
		return;
	}

	addMove(game, move);
}

static bool HandleMove(HWND hWnd, GameInstance* game, uint8_t currSquare, uint8_t sourceNr, uint8_t column, uint8_t row) {
	if (!ACTION_SELECTED(g_ActionState)) {
		return false;
	}

	MoveArray* moves  = &game->state.validMoves[sourceNr];
	Square*	   target = &game->state.board.squares[column][row];

	if (moves->size == 0) {
		return false;
	}

	for (int i = 0; i < moves->size; i++) {
		if (currSquare == MOVE_TARGET(moves->moves[i])) {
			MoveDraggedPiece(hWnd, game, target, moves->moves[i]);
			g_ActionState = 0;
			g_renderPiecesBuffer = 1;
			ClearBufferByType(hWnd, BUFFER_CURSOR);
			return true;
		}
	}

	return false;
}

static void CopyFromBackBufferToDDC(HDC hDC) {
	HDC hBackBuffer = GetBackBuffer(hDC);

	BitBlt(hDC, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT, hBackBuffer, 0, 0, SRCCOPY);
}

static void HandlePromotionPaint(HWND hWnd, HDC hDC) {	
	if (g_hoverBitBoard) {
		return;
	}
	
	CopyFromBackBufferToDDC(hDC);

	g_allowWindowRendering = 0;
}

static void HandleWindowPaint(HWND hWnd, HDC hDC) {
	if (!g_allowWindowRendering) {
		return;
	}

	GameInstance* game = getGameInstance();
	if (!game) {
		return;
	}

	if (g_promotionMode == 1) {
		HandlePromotionPaint(hWnd, hDC);
		return;
	}

	if (g_renderPiecesBuffer) {
		ClearBufferByType(hWnd, BUFFER_PIECE_BG);
		ClearBufferByType(hWnd, BUFFER_PIECES);

		if (GAME_FLAG_CHECK(game->state.flags)) {
			HDC hPiecesBgBuffer = GetPieceBgBuffer(hDC);
			RenderCheck(hPiecesBgBuffer, &game->state.board, game->state.flags);
		}

		RenderPieces(g_buffers[BUFFER_PIECES].hMemDC, &game->state.board);
		g_renderPiecesBuffer = 0;
	}

	if (g_renderHoverBuffer) {
		COLORREF hoverColor = RGB(
			g_sqHoverColor.red,
			g_sqHoverColor.green,
			g_sqHoverColor.blue
		);

		DrawPossibleMoves(hWnd, game->state.validMoves, ACTION_SOURCE_NR(g_ActionState));
		g_renderHoverBuffer = 0;
	}

	HDC hBackBuffer = GetBackBuffer(hDC);
	ComposeBuffers(hDC);

	BitBlt(hDC, 0, 0, GAMEWND_WIDTH, GAMEWND_HEIGHT, hBackBuffer, 0, 0, SRCCOPY);

	g_allowWindowRendering = 0;
}

static bool isSquareNrInMoves(BYTE squareNr, MoveArray* moves) {
	for (int i = 0; i < moves->size; i++) {
		if (squareNr != MOVE_TARGET(moves->moves[i])) {
			continue;
		}
		return true;
	}
	return false;
}

static bool isCursorInSquare(int cursX, int cursY, BYTE column, BYTE row) {
	if ((cursX / g_fieldSize) == column && (cursY / g_fieldSize) == row) {
		return true;
	}
	return false;
}

static bool RenderPromotionHover(HWND hWnd, int cursX, int cursY, BYTE currSquare, BYTE targetCol, BYTE targetRow) {
	if (currSquare == log2(g_hoverBitBoard)) {
		return false;
	}

	ChessColor color = targetRow == 0 ? WHITE : BLACK;
	INT8 direction = color == WHITE ? 1 : -1;

	HDC hDC;
	BYTE currentRow = targetRow;
	for (int i = 0; i < 4; i++) {
		currentRow = targetRow + i * direction;

		if (!isCursorInSquare(cursX, cursY, targetCol, currentRow)) {
			continue;
		}
		
		hDC = GetDC(hWnd);

		CopyFromBackBufferToDDC(hDC);
		CopySpriteToBuffer(hDC, SPRITE_CAPTURE, targetCol, currentRow);
		g_hoverBitBoard = ((1ULL) << currSquare);

		ReleaseDC(hWnd, hDC);
		return true;
	}

	ClearGameWnd(hWnd);
	g_hoverBitBoard = 0;
	return false;
}

static void HandlePromotionMove(int cursX, int cursY, BYTE currSquare, BYTE targetCol, BYTE targetRow) {
	BYTE currentRow = targetRow;

	ChessColor color = targetRow == 0 ? WHITE : BLACK;
	INT8 direction = color == WHITE ? 1 : -1;

	for (int i = 0; i < 4; i++) {
		currentRow = targetRow + i * direction;

		if (!isCursorInSquare(cursX, cursY, targetCol, currentRow)) {
			continue;
		}

		GameInstance* game  = getGameInstance();
		PieceType promotion = QUEEN + i;

		addPromotionToMove(&g_promotionMove, promotion);
		if (addMove(game, g_promotionMove)) {
			g_promotionMove = 0;
			break;
		}
	}
}

static void PromotionInputHandler(HWND hWnd, GameInstance* game, int cursX, int cursY, UINT winMsg) {
	BYTE targetNr  = MOVE_TARGET(g_promotionMove);
	BYTE targetCol = targetNr % BOARD_COLS;
	BYTE targetRow = BOARD_ROWS - 1 - targetNr / BOARD_ROWS;

	BYTE hoveredNr = (BOARD_ROWS - 1 - (cursY / g_fieldSize)) * BOARD_ROWS + (cursX / g_fieldSize);

	switch (winMsg) {
	case WM_MOUSEMOVE: {
		RenderPromotionHover(hWnd, cursX, cursY, hoveredNr, targetCol, targetRow);
		break;
	}
	case WM_LBUTTONDOWN: {
		HandlePromotionMove(cursX, cursY, hoveredNr, targetCol, targetRow);
		g_promotionMode = 0;
		ClearGameWnd(hWnd);
		break;
	}
	case WM_RBUTTONDOWN: {
		g_promotionMode = 0;
		ClearGameWnd(hWnd);
		break;
	}
	}
}

static void MouseInputHandler(HWND hWnd, LPARAM lParam, UINT winMsg) {
	Square* square = NULL;
	int		cursX  = GET_X_LPARAM(lParam);
	int		cursY  = GET_Y_LPARAM(lParam);
	BYTE	column = cursX / g_fieldSize;
	BYTE	row	   = cursY / g_fieldSize;
	
	GameInstance* game = getGameInstance();
	
	if (g_promotionMode == 1) {
		PromotionInputHandler(hWnd, game, cursX, cursY, winMsg);
		return;
	}

	bool isPosOnBoard = isPositionOnBoard(&game->state.board, column, row);
	if (isPosOnBoard) {
		square = &game->state.board.squares[row][column];
	}

	bool isSelection = ACTION_SELECTED(g_ActionState) == 1;
	bool isDragging  = ACTION_DRAGGING(g_ActionState) == 1;
	BYTE sourceNr	 = ACTION_SOURCE_NR(g_ActionState);

	switch (winMsg) {
	case WM_MOUSEMOVE: {
		if (!isSelection) {
			return;
		}

		if (isDragging) {
			CopyPieceToCursorBuffer(hWnd, cursX, cursY);
		}

		g_renderHover = false;
		g_hoverBitBoard = 0;
		
		if (!square) {
			return;
		}

		if (log2(g_hoverBitBoard) == square->nr) {
			return;
		}

		if (isSquareNrInMoves(square->nr, &game->state.validMoves[sourceNr])) {
			g_renderHover = true;
			g_hoverBitBoard = ((1ULL) << square->nr);
			break;
		}
		break;
	}
	case WM_LBUTTONDOWN: {
		if (!isPosOnBoard) {
			return;
		}

		if (!square || !isValidChessPiece(&square->piece)) {
			return;
		}

		ChessColor currentPlayer = GAME_FLAG_WHITE_MOVES(game->state.flags) ? WHITE : BLACK;
		if (isOppositePieceColor(currentPlayer, square->piece.color)) {
			return;
		}

		if (isSelection && !isDragging) {
			if (square->nr == sourceNr && g_pieceAlreadySelected) {
				g_ActionState ^= IS_DRAGGING;
				break;
			}

			HandleMove(hWnd, game, square->nr, sourceNr, column, row);
			g_ActionState = 0;
			g_renderHover = 0;
			g_pieceAlreadySelected = false;
			break;
		}

		SetInitActionState(column, row, square->piece.color);
		g_renderHoverBuffer = 1;

		SetCapture(hWnd);
		break;
	}
	case WM_LBUTTONUP: {
		if (!isSelection) {
			return;
		}

		ReleaseCapture();
		ClearBufferByType(hWnd, BUFFER_CURSOR);

		if (square) {
			if (square->nr == sourceNr) {
				if (g_pieceAlreadySelected) {
					g_ActionState = 0;
					g_pieceAlreadySelected = false;
					break;
				}

				g_ActionState ^= IS_DRAGGING;

				CopyPieceToCursorBuffer(hWnd,
					column * g_fieldSize + g_fieldSize / 2,
					row    * g_fieldSize + g_fieldSize / 2
				);

				g_pieceAlreadySelected = true;
				break;
			}
		
			HandleMove(hWnd, game, square->nr, sourceNr, column, row);
		}

		g_ActionState = 0;
		g_renderHover = 0;
		g_pieceAlreadySelected = false;
		break;
	}
	case WM_RBUTTONDOWN: {
		if (!isSelection) {
			return;
		}
		ReleaseCapture();
		ClearBufferByType(hWnd, BUFFER_CURSOR);

		g_ActionState = 0;
		g_renderHover = 0;
		g_pieceAlreadySelected = false;
		break;
	}
	}
	ClearGameWnd(hWnd);
}

LRESULT CALLBACK GameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HDC hDC;
	PAINTSTRUCT paintStruct;
	int winMsgId, winMsgEvent;

	switch (message) {
	case WM_CREATE: {
		break;
	}
	case WM_ERASEBKGND: {
		g_allowWindowRendering = 1;
		return 0;
	}
	case WM_PAINT: {
		hDC = BeginPaint(hWnd, &paintStruct);

		HandleWindowPaint(hWnd, hDC);

		EndPaint(hWnd, &paintStruct);
		ReleaseDC(hWnd, hDC);
		break;
	} 
	case WM_MOUSEMOVE: 
	case WM_LBUTTONDOWN: 
	case WM_LBUTTONUP: 
	case WM_RBUTTONDOWN: {
		MouseInputHandler(hWnd, lParam, message);
		break;
	}
	case WM_COMMAND: {
		winMsgId = LOWORD(wParam);
		winMsgEvent = HIWORD(wParam);

		switch (LOWORD(wParam)) {
		case ID_FILE_EXIT: {
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
		default:
			break;
		}
		break;
	}
	case WM_CLOSE: {
		DestroyWindow(hWnd);
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		break;
	}
	default: {
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	}

	return 0;
}

void TerminateGameWnd() {
	for (int i = 0; i < BUFFER_COUNT; i++) {
		if (g_buffers[i].hMemDC) {
			DeleteDC(g_buffers[i].hMemDC);
			g_buffers[i].hMemDC = NULL;
		}
		if (g_buffers[i].hBuffer) {
			DeleteObject(g_buffers[i].hBuffer);
			g_buffers[i].hBuffer = NULL;
		}
		g_buffers[i].width  = 0;
		g_buffers[i].height = 0;
	}

	if (g_hBrushChessWhite) {
		DeleteObject(g_hBrushChessWhite);
		g_hBrushChessWhite = NULL;
	}
	if (g_hBrushChessBlack) {
		DeleteObject(g_hBrushChessBlack);
		g_hBrushChessBlack = NULL;
	}
}