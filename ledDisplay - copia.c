#include "ledDisplay.h"

// Maquina de estados: lista de transiciones
// {EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
fsm_trans_t fsm_trans_excitacion_display[] = {
	{ DISPLAY_ESPERA_COLUMNA, CompruebaTimeoutColumnaDisplay, DISPLAY_ESPERA_COLUMNA, ActualizaExcitacionDisplay },
	{-1, NULL, -1, NULL },
};

//------------------------------------------------------
// PROCEDIMIENTOS DE INICIALIZACION DE LOS OBJETOS ESPECIFICOS
//------------------------------------------------------

void InicializaLedDisplay (TipoLedDisplay *led_display) {

	for(int i=0; i<NUM_FILAS_DISPLAY; i++){
		pinMode(led_display->filas[i], OUTPUT);
		digitalWrite(led_display->filas[i], HIGH);
	}

	for(int i=0; i<NUM_PINES_CONTROL_COLUMNAS_DISPLAY; i++){
		pinMode(led_display->pines_control_columnas[i], OUTPUT);
	}

	led_display->tmr_refresco_display = tmr_new(timer_refresco_display_isr);
	tmr_startms(led_display->tmr_refresco_display, TIMEOUT_COLUMNA_DISPLAY);
}

//------------------------------------------------------
// OTROS PROCEDIMIENTOS PROPIOS DE LA LIBRERIA
//------------------------------------------------------

void ApagaFilas (TipoLedDisplay *led_display){
	for(int i=0; i<NUM_FILAS_DISPLAY; i++){
		digitalWrite(led_display->filas[i], HIGH); //Activo a nivel bajo
	}
}

void ExcitaColumnas(int columna) {

	switch(columna) {
	case 0: //000
		digitalWrite(led_display.pines_control_columnas[2], LOW);
		digitalWrite(led_display.pines_control_columnas[1], LOW);
		digitalWrite(led_display.pines_control_columnas[0], LOW);
		break;
	case 1: //001
		digitalWrite(led_display.pines_control_columnas[2], LOW);
		digitalWrite(led_display.pines_control_columnas[1], LOW);
		digitalWrite(led_display.pines_control_columnas[0], HIGH);
		break;
	case 2: //010
		digitalWrite(led_display.pines_control_columnas[2], LOW);
		digitalWrite(led_display.pines_control_columnas[1], HIGH);
		digitalWrite(led_display.pines_control_columnas[0], LOW);
		break;
	case 3: //011
		digitalWrite(led_display.pines_control_columnas[2], LOW);
		digitalWrite(led_display.pines_control_columnas[1], HIGH);
		digitalWrite(led_display.pines_control_columnas[0], HIGH);
		break;
	case 4: //100
		digitalWrite(led_display.pines_control_columnas[2], HIGH);
		digitalWrite(led_display.pines_control_columnas[1], LOW);
		digitalWrite(led_display.pines_control_columnas[0], LOW);
		break;
	case 5: //101
		digitalWrite(led_display.pines_control_columnas[2], HIGH);
		digitalWrite(led_display.pines_control_columnas[1], LOW);
		digitalWrite(led_display.pines_control_columnas[0], HIGH);
		break;
	case 6: //110
		digitalWrite(led_display.pines_control_columnas[2], HIGH);
		digitalWrite(led_display.pines_control_columnas[1], HIGH);
		digitalWrite(led_display.pines_control_columnas[0], LOW);
		break;
	case 7: //111
		digitalWrite(led_display.pines_control_columnas[2], HIGH);
		digitalWrite(led_display.pines_control_columnas[1], HIGH);
		digitalWrite(led_display.pines_control_columnas[0], HIGH);
		break;
	default:
		printf("Número de columna no válido.\n");
		fflush(stdout);
		break;
	}
}

void ActualizaLedDisplay (TipoLedDisplay *led_display) {
	ApagaFilas(led_display);
	ExcitaColumnas(led_display->p_columna);
	for(int i=0; i<NUM_FILAS_DISPLAY; i++){
		if(led_display->pantalla.matriz[led_display->p_columna][i]==1 || led_display->pantalla.matriz[led_display->p_columna][i]==4){
			digitalWrite(led_display->filas[i], LOW); //Fila a LOW: Activa
		}
	}
}

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------

int CompruebaTimeoutColumnaDisplay (fsm_t* this) {
	int result = 0;
	TipoLedDisplay *p_ledDisplay;
	p_ledDisplay = (TipoLedDisplay*)(this->user_data);

	piLock(MATRIX_KEY);
	result = p_ledDisplay->flags & FLAG_TIMEOUT_COLUMNA_DISPLAY;
	piUnlock(MATRIX_KEY);

	return result;
}

//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------

void ActualizaExcitacionDisplay (fsm_t* this) {
	TipoLedDisplay *p_ledDisplay;
	p_ledDisplay = (TipoLedDisplay*)(this->user_data);

	piLock(MATRIX_KEY);
	p_ledDisplay->flags &= ~FLAG_TIMEOUT_COLUMNA_DISPLAY;
	ActualizaLedDisplay(p_ledDisplay);
	p_ledDisplay->p_columna++;
	if(p_ledDisplay->p_columna >= NUM_COLUMNAS_DISPLAY) p_ledDisplay->p_columna = 0;
	piUnlock(MATRIX_KEY);

	tmr_startms(p_ledDisplay->tmr_refresco_display, TIMEOUT_COLUMNA_DISPLAY);
}

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------

void timer_refresco_display_isr (union sigval value) {
	piLock(MATRIX_KEY);
	led_display.flags |= FLAG_TIMEOUT_COLUMNA_DISPLAY;
	piUnlock(MATRIX_KEY);
}
