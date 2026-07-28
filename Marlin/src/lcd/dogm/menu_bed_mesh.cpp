#include "../../inc/MarlinConfig.h"

#if HAS_MARLINUI_U8GLIB && ENABLED(BED_MESH_VIEWER)

#include "../marlinui.h"
#include "marlinui_DOGM.h"

// Inclusione ufficiale per la classe 'bedlevel' (mesh_bed_leveling)
#include "../../feature/bedlevel/bedlevel.h"

#define MESH_MAP_COLS _MIN(GRID_MAX_POINTS_X, 7)
#define MESH_MAP_ROWS _MIN(GRID_MAX_POINTS_Y, 7)

static int16_t static_micron_values[7][7]; 
static bool view_mode_numeric = false;
static bool view_mode_help = false; 
static uint8_t selected_button = 1; // 1 = VIS, 2 = INFO, 3 = ESC

void menu_bed_mesh_draw();

// 1. INIZIALIZZAZIONE ALL'APERTURA DELLA PAGINA
void menu_bed_mesh_init() {
  selected_button = 1;       
  view_mode_numeric = false;  
  view_mode_help = false;
  
  for (uint8_t x = 0; x < 7; x++) {
    for (uint8_t y = 0; y < 7; y++) {
      static_micron_values[x][y] = 0;
    }
  }

  // Estrazione dati per Mesh Bed Leveling Manual
  for (uint8_t x = 0; x < MESH_MAP_COLS; x++) {
    for (uint8_t y = 0; y < MESH_MAP_ROWS; y++) {
      float z = 0.0;
      
      #if ENABLED(MESH_BED_LEVELING)
        z = bedlevel.z_values[x][y]; 
      #elif ENABLED(AUTO_BED_LEVELING_BILINEAR)
        z = z_values[x][y];
      #elif ENABLED(AUTO_BED_LEVELING_UBL)
        z = ubl.z_values[x][y];
      #endif
      
      if (!isnan(z) && z > -10.0 && z < 10.0) {
        static_micron_values[x][y] = (int16_t)(z * 1000.0);
      } else {
        static_micron_values[x][y] = 0;
      }
    }
  }

  ui.encoderPosition = 0;

  // LOOP ESCLUSIVO CON TRACCIAMENTO RELATIVO DELL'ENCODER
  bool in_mesh_screen = true;
  int16_t last_encoder_pos = 0;
  
  while (in_mesh_screen) {
    ui.update(); 

    const int16_t current_encoder_pos = ui.encoderPosition;
    if (current_encoder_pos != last_encoder_pos) {
      int16_t diff = current_encoder_pos - last_encoder_pos;
      
      if (view_mode_help) {
        view_mode_help = false;
        ui.encoderPosition = 0;
        last_encoder_pos = 0;
      } 
      else {
        if (diff > 0) {
          selected_button = (selected_button % 3) + 1;
        } 
        else if (diff < 0) {
          selected_button = (selected_button == 1) ? 3 : selected_button - 1;
        }
        last_encoder_pos = current_encoder_pos;
      }
      ui.refresh(); 
    }

    if (ui.use_click()) {
      if (view_mode_help) {
        view_mode_help = false;
      } 
      else {
        if (selected_button == 1) {
          view_mode_numeric = !view_mode_numeric;
        } 
        else if (selected_button == 2) {
          view_mode_help = true; 
        }
        else if (selected_button == 3) {
          in_mesh_screen = false; 
        }
      }
      ui.refresh();
    }

    u8g.firstPage();
    do {
      menu_bed_mesh_draw();
    } while (u8g.nextPage());

    safe_delay(20); 
  }

  ui.goto_previous_screen();
}

// 2. FUNZIONE DI RENDERING GRAFICO
void menu_bed_mesh_draw() {
  
  if (view_mode_help) {
    // =========================================================================
    // INF SCREEN INFOGRAFICO UNIVERSALE (SENZA TESTO / SCHERMO INTERO)
    // =========================================================================
    
    // 1. DISEGNO DELLA SEZIONE LATERALE DEL PIATTO A SEGMENTI (Sostituisce drawBezier)
    // Linea tratteggiata di base 0 (Livello ideale del piatto)
    for (int x = 6; x < 50; x += 3) u8g.drawHLine(x, 26, 1);
    
    // Profilo deformato del piatto: Picco positivo verso l'alto (Gobba)
    u8g.drawLine(6, 26, 10, 18);
    u8g.drawLine(10, 18, 16, 14);
    u8g.drawLine(16, 14, 22, 18);
    u8g.drawLine(22, 18, 26, 26);

    // Profilo deformato del piatto: Lacuna negativa verso il basso (Conca)
    u8g.drawLine(26, 26, 30, 34);
    u8g.drawLine(30, 34, 36, 38);
    u8g.drawLine(36, 38, 42, 34);
    u8g.drawLine(42, 34, 46, 26);

    // Frecce indicatrici per collegare i picchi ai simboli di destra
    u8g.drawLine(18, 10, 75, 10); u8g.drawPixel(18, 11); u8g.drawPixel(18, 12); // Freccia su
    u8g.drawLine(38, 42, 75, 42); u8g.drawPixel(38, 41); u8g.drawPixel(38, 40); // Freccia giù

    // 2. RAPPRESENTAZIONE VISIVA DELLE MATRICI REALI (A DESTRA)
    // Centro logico dei due campioni esplicativi della legenda
    const int ex_pos_center_x = 80;
    const int ex_neg_center_x = 80;
    const int ex_pos_center_y = 14;
    const int ex_neg_center_y = 42;

    // Esempio Positivo (In alto): Piatto alto -> Espansione a centro stabile (Disegnamo 8 pixel di esempio)
    u8g.drawPixel(ex_pos_center_x, ex_pos_center_y);
    const int8_t ex_path_pos_x[] = {  0, 1, 0, -1, 1, 1, -1, -1 };
    const int8_t ex_path_pos_y[] = { -1, 0, 1,  0, -1, 1,  1, -1 };
    for (int i = 0; i < 8; i++) u8g.drawPixel(ex_pos_center_x + ex_path_pos_x[i], ex_pos_center_y + ex_path_pos_y[i]);

    // Esempio Negativo (In basso): Piatto basso -> Cornice 9x9 con la tua sequenza invertita (8 pixel di esempio)
    u8g.drawFrame(ex_neg_center_x - 4, ex_neg_center_y - 4, 9, 9);
    const int8_t ex_path_neg_x[] = { -3, -3, 3, 3, -2, -3, -3, -2 };
    const int8_t ex_path_neg_y[] = { -3,  3, 3, -3, -3, -2, 2, 3 };
    for (int i = 0; i < 8; i++) u8g.drawPixel(ex_neg_center_x + ex_path_neg_x[i], ex_neg_center_y + ex_path_neg_y[i]);

    // 3. RAPPRESENTAZIONE FORMULA MATEMATICA DINAMICA (IN BASSO A SINISTRA)
    #ifdef MESH_EDIT_Z_STEP
      const int16_t step_micron = (int16_t)(MESH_EDIT_Z_STEP * 1000.0);
    #else
      const int16_t step_micron = 25;
    #endif

    char step_buf[6];
    itoa(step_micron, step_buf, 10);

    u8g.setFont(u8g_font_5x7);
    u8g.drawStr(6, 60, "1 Px =");
    u8g.drawStr(44, 60, step_buf);
    
    int label_offset_x = 44 + (strlen(step_buf) * 6);
    u8g.drawVLine(label_offset_x, 56, 4);      
    u8g.drawVLine(label_offset_x + 2, 56, 4);  
    u8g.drawHLine(label_offset_x, 59, 3);      
    u8g.drawPixel(label_offset_x - 1, 59);     
    u8g.drawStr(label_offset_x + 4, 60, "m");  

    // =========================================================================
    // 4. LA TUA NUOVA ISTRUZIONE DI USCITA ICONICA VERTICALE (A DESTRA)
    // =========================================================================
    const int exit_x = 112; // Asse X centrato per la colonna dei simboli a destra

    // A. POMELLO IN ALTO (Y: 2-10)
    u8g.drawCircle(exit_x, 6, 4); 
    u8g.drawPixel(exit_x, 6);

    // B. FRECCIA CHE INDICA IL POMELLO (Y: 14-19)
    u8g.drawVLine(exit_x, 14, 5); 
    u8g.drawPixel(exit_x - 1, 17); u8g.drawPixel(exit_x + 1, 17);
    u8g.drawPixel(exit_x - 2, 16); u8g.drawPixel(exit_x + 2, 16);

    // C. PAROLA "CLICK" CENTRATA SOTTO LA FRECCIA (Y: 22-29)
    // Il font 5x7 occupa 5 pixel per carattere. "CLICK" è lunga 29 pixel.
    u8g.setFont(u8g_font_5x7);
    u8g.drawStr(exit_x - 14, 29, "CLICK");

    // D. ICONA DELLA PORTA APERTA CON FRECCIA DI USCITA (Y: 36-58)
    const int door_x = exit_x - 8; // Spostiamo leggermente per centrare il disegno della porta
    const int door_y = 38;

    // Disegno della struttura della porta (Stipite sinistro, superiore e destro)
    u8g.drawVLine(door_x, door_y, 16);          // Stipite sinistro della porta chiusa/cornice
    u8g.drawHLine(door_x, door_y, 12);          // Stipite superiore
    u8g.drawVLine(door_x + 12, door_y, 16);     // Stipite destro
    
    // Disegno dell'anta aperta (linea obliqua che simula l'apertura prospettica verso sinistra)
    u8g.drawLine(door_x, door_y, door_x - 4, door_y + 4);
    u8g.drawVLine(door_x - 4, door_y + 4, 16);
    u8g.drawLine(door_x - 4, door_y + 20, door_x, door_y + 16);

    // Freccia che attraversa la porta uscendo verso destra
    u8g.drawHLine(door_x + 3, door_y + 9, 13);   // Asta della freccia che esce dalla porta
    u8g.drawPixel(door_x + 14, door_y + 8);      // Punta su della freccia
    u8g.drawPixel(door_x + 14, door_y + 10);     // Punta giù della freccia
    u8g.drawPixel(door_x + 13, door_y + 7);
    u8g.drawPixel(door_x + 13, door_y + 11);

    return; // Interrompe il disegno standard di Marlin
  }

  // =========================================================================
  // LOGICA STANDARD DELLA MAPPA (GRIGLIA + PULSANTI LATERALI)
  // =========================================================================
  if (view_mode_numeric) {
    u8g.setFont(u8g_font_5x7); 

    const int num_x_spacing = 90 / MESH_MAP_COLS;  
    const int num_y_spacing = 50 / MESH_MAP_ROWS;   
    const int start_num_x = 6 + (90 - (MESH_MAP_COLS * num_x_spacing)) / 2;
    const int start_num_y = (64 - (MESH_MAP_ROWS * num_y_spacing)) / 2;

    for (uint8_t x = 0; x < MESH_MAP_COLS; x++) {
      for (uint8_t y = 0; y < MESH_MAP_ROWS; y++) {
        int16_t micron_val = static_micron_values[x][y];
        int cell_center_x = start_num_x + (x * num_x_spacing) + (num_x_spacing / 2);
        int draw_y = start_num_y + ((MESH_MAP_ROWS - 1 - y) * num_y_spacing) + 8; 
        
        char buf[6]; 

        if (micron_val == 0) {
          u8g.drawStr(cell_center_x - 2, draw_y, "0");
        }
        else if (micron_val < 0) {
          micron_val = -micron_val; 
          itoa(micron_val, buf, 10);
          int text_len = strlen(buf);
          int text_width = text_len * 5 + (text_len - 1); 
          int draw_x = cell_center_x - (text_width / 2);  

          u8g.drawBox(draw_x - 1, draw_y - 7, text_width + 2, 9);
          u8g.setColorIndex(0);
          u8g.drawStr(draw_x, draw_y, buf);
          u8g.setColorIndex(1); 
        } 
        else {
          itoa(micron_val, buf, 10);
          int text_len = strlen(buf);
          int text_width = text_len * 5 + (text_len - 1);
          int draw_x = cell_center_x - (text_width / 2); 
          u8g.drawStr(draw_x, draw_y, buf);
        }
      }
    }
  } 
  else {
    const int grid_y_spacing = 50 / MESH_MAP_ROWS;   
    const int grid_x_spacing = grid_y_spacing; 

    const int total_grid_w = MESH_MAP_COLS * grid_x_spacing;
    const int start_x = (100 - total_grid_w) / 2;          
    const int start_y = (64 - (MESH_MAP_ROWS * grid_y_spacing)) / 2;          

    const int8_t path_pos_x[] = {  0, 1, 0, -1, 1, 1, -1, -1,  0, 2, 0, -2, 1, 2, 2, 1, -1, -2, -2, -1,  0, 2, 3, 2, 0, -2, -3, -2, 1, 3, 3, 1, -1, -3, -3, -1, 2, 3, 3, 2, -2, -3, -3, -2, 3, 3, -3, -3 };
    const int8_t path_pos_y[] = { -1, 0, 1,  0, -1, 1,  1, -1, -2, 0, 2,  0, -2, -1, 1, 2,  2, 1, -1, -2, -3, -2, 0, 2, 3, 2, 0, -2, -3, -1, 1, 3, 3, 1, -1, -3, -3, -2, 2, 3, 3, 2, -2, -3, -3, 3, 3, -3 };
    const int8_t path_neg_x[] = { -3, -3, 3, 3, -2, -3, -3, -2, 2, 3, 3, 2, -1, -3, -3, -1, 1, 3, 3, 1, -2, -3, -2, 0, 2, 3, 2, 0, -1, -2, -2, -1, 1, 2, 2, 1, -2, 0, 2, 0, -1, -1, 1, 1, -1, 0, 1, 0 };
    const int8_t path_neg_y[] = { -3,  3, 3, -3, -3, -2, 2, 3, 3, 2, -2, -3, -3, -1, 1, 3, 3, 1, -1, -3, -2, 0, 2, 3, 2, 0, -2, -3, -2, -1, 1, 2, 2, 1, -1, -2, 0, 2, 0, -2, -1, 1, 1, -1, 0, 1, 0, -1 };

    for (uint8_t x = 0; x < MESH_MAP_COLS; x++) {
      for (uint8_t y = 0; y < MESH_MAP_ROWS; y++) {
        int16_t val = static_micron_values[x][y]; 
        
        int center_x = start_x + (x * grid_x_spacing) + (grid_x_spacing / 2);
        int center_y = start_y + ((MESH_MAP_ROWS - 1 - y) * grid_y_spacing) + (grid_y_spacing / 2);

        #ifdef MESH_EDIT_Z_STEP
          const int16_t step_micron = (int16_t)(MESH_EDIT_Z_STEP * 1000.0);
        #else
          const int16_t step_micron = 25;
        #endif

        int pixels_to_draw = (int)(abs(val) / (step_micron > 0 ? step_micron : 25));
        if (pixels_to_draw > 48) pixels_to_draw = 48; 

        if (pixels_to_draw == 0) {
          u8g.drawPixel(center_x, center_y); 
        } 
        else {
          if (val > 0) {
            u8g.drawPixel(center_x, center_y); 
            for (int i = 0; i < pixels_to_draw; i++) {
              u8g.drawPixel(center_x + path_pos_x[i], center_y + path_pos_y[i]);
            }
          } 
          else {
            u8g.drawFrame(center_x - 4, center_y - 4, 9, 9);
            for (int i = 0; i < pixels_to_draw; i++) {
              u8g.drawPixel(center_x + path_neg_x[i], center_y + path_neg_y[i]);
            }
          }
        }
      }
    }
  }

  // INTERFACCIA MENÙ LATERALE VERTICALE A 3 PULSANTI (X: 102-128)
  u8g.setFont(u8g_font_6x10); 
  u8g.drawVLine(101, 0, 64); 

  // BOTTONE 1: VIS (Pixel Y: 2-15)
  if (selected_button == 1) {
    u8g.drawBox(103, 2, 23, 13);       
    u8g.setColorIndex(0);              
    u8g.drawStr(106, 12, "VIS");
    u8g.setColorIndex(1);              
  } else {
    u8g.drawFrame(103, 2, 23, 13);     
    u8g.drawStr(106, 12, "VIS");
  }

  // BOTTONE 2: INF (Pixel Y: 24-37)
  if (selected_button == 2) {
    u8g.drawBox(103, 24, 23, 13);      
    u8g.setColorIndex(0);              
    u8g.drawStr(106, 34, "INF");
    u8g.setColorIndex(1);              
  } else {
    u8g.drawFrame(103, 24, 23, 13);    
    u8g.drawStr(106, 34, "INF");
  }

  // BOTTONE 3: ESC (Pixel Y: 46-59)
  if (selected_button == 3) {
    u8g.drawBox(103, 46, 23, 13);      
    u8g.setColorIndex(0);              
    u8g.drawStr(106, 56, "ESC");
    u8g.setColorIndex(1);              
  } else {
    u8g.drawFrame(103, 46, 23, 13);    
    u8g.drawStr(106, 56, "ESC");
  }
}

#endif // HAS_MARLINUI_U8GLIB && ENABLED(BED_MESH_VIEWER)