//////////////////////////////////////////////////////////////////////////////////
//                                                                              //
// StyleAsCode exporter v2.0 - Style data exported as a values array            //
//                                                                              //
// USAGE: On init call: GuiLoadStyleBlissfulOrange();                                   //
//                                                                              //
// more info and bugs-report:  github.com/raysan5/raygui                        //
// feedback and support:       ray[at]raylibtech.com                            //
//                                                                              //
// Copyright (c) 2020-2025 raylib technologies (@raylibtech)                    //
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

#define BLISSFUL_ORANGE_STYLE_PROPS_COUNT  17

// Custom style name: blissful_orange
static const GuiStyleProp blissful_orangeStyleProps[BLISSFUL_ORANGE_STYLE_PROPS_COUNT] = {
    { 0, 0, (int)0xd17729ff },    // DEFAULT_BORDER_COLOR_NORMAL 
    { 0, 1, (int)0x2d1c0dff },    // DEFAULT_BASE_COLOR_NORMAL 
    { 0, 2, (int)0xd17729ff },    // DEFAULT_TEXT_COLOR_NORMAL 
    { 0, 3, (int)0xd96902ff },    // DEFAULT_BORDER_COLOR_FOCUSED 
    { 0, 4, (int)0xc9a158ff },    // DEFAULT_BASE_COLOR_FOCUSED 
    { 0, 5, (int)0xffffffff },    // DEFAULT_TEXT_COLOR_FOCUSED 
    { 0, 6, (int)0xb08758ff },    // DEFAULT_BORDER_COLOR_PRESSED 
    { 0, 7, (int)0xd19968ff },    // DEFAULT_BASE_COLOR_PRESSED 
    { 0, 8, (int)0xf1e6d9ff },    // DEFAULT_TEXT_COLOR_PRESSED 
    { 0, 9, (int)0xd1baa7ff },    // DEFAULT_BORDER_COLOR_DISABLED 
    { 0, 10, (int)0x2d2924ff },    // DEFAULT_BASE_COLOR_DISABLED 
    { 0, 11, (int)0xd1baa7ff },    // DEFAULT_TEXT_COLOR_DISABLED 
    { 0, 16, (int)0x00000018 },    // DEFAULT_TEXT_SIZE 
    { 0, 17, (int)0x00000003 },    // DEFAULT_TEXT_SPACING 
    { 0, 18, (int)0xd17729ff },    // DEFAULT_LINE_COLOR 
    { 0, 19, (int)0x2d1c0dff },    // DEFAULT_BACKGROUND_COLOR 
    { 0, 20, (int)0x00000024 },    // DEFAULT_TEXT_LINE_SPACING 
};

// Style loading function: blissful_orange
static void GuiLoadStyleBlissfulOrange(void)
{
    // Load style properties provided
    // NOTE: Default properties are propagated
    for (int i = 0; i < BLISSFUL_ORANGE_STYLE_PROPS_COUNT; i++)
    {
        GuiSetStyle(blissful_orangeStyleProps[i].controlId, blissful_orangeStyleProps[i].propertyId, blissful_orangeStyleProps[i].propertyValue);
    }

    //-----------------------------------------------------------------

    // TODO: Custom user style setup: Set specific properties here (if required)
    // i.e. Controls specific BORDER_WIDTH, TEXT_PADDING, TEXT_ALIGNMENT
}
