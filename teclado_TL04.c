#include "teclado_TL04.h"

char tecladoTL04[4][4] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'E', '0', 'F', 'D'}
};

// Maquina de estados: lista de transiciones
// {EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
fsm_trans_t fsm_trans_excitacion_columnas[] = {
	{ TECLADO_ESPERA_COLUMNA, CompruebaTimeoutColumna, TECLADO_ESPERA_COLUMNA, TecladoExcitaColumna },
	{-1, NULL, -1, NULL },
};

fsm_trans_t fsm_trans_deteccion_pulsaciones[] = {
	{ TECLADO_ESPERA_TECLA, CompruebaTeclaPulsada, TECLADO_ESPERA_TECLA, ProcesaTeclaPulsada},
	{-1, NULL, -1, NULL },
};

//------------------------------------------------------
// PROCEDIMIENTOS DE INICIALIZACION DE LOS OBJETOS ESPECIFICOS
//------------------------------------------------------

void InicializaTeclado(TipoTeclado *p_teclado) {

	for(int i=0; i<4; i++){
		pinMode(p_teclado->filas[i], INPUT);
		pullUpDnControl(p_teclado->filas[i], PUD_DOWN);
		wiringPiISR(p_teclado->filas[i], INT_EDGE_RISING, p_teclado->rutinas_ISR[i]);
	}

	for(int i=0; i<4; i++){
		pinMode(p_teclado->columnas[i], OUTPUT);
		digitalWrite(p_teclado->columnas[i], LOW);
	}

	p_teclado->tmr_duracion_columna = tmr_new (timer_duracion_columna_isr);
	tmr_startms(p_teclado->tmr_duracion_columna, TIMEOUT_COLUMNA_TECLADO);
}

//------------------------------------------------------
// OTROS PROCEDIMIENTOS PROPIOS DE LA LIBRERIA
//------------------------------------------------------

void ActualizaExcitacionTecladoGPIO (int columna) {
	switch(columna){
	case COLUMNA_1:
		digitalWrite(teclado.columnas[3], LOW);
		digitalWrite(teclado.columnas[0], HIGH);
		break;
	case COLUMNA_2:
		digitalWrite(teclado.columnas[0], LOW);
		digitalWrite(teclado.columnas[1], HIGH);
		break;
	case COLUMNA_3:
		digitalWrite(teclado.columnas[1], LOW);
		digitalWrite(teclado.columnas[2], HIGH);
		break;
	case COLUMNA_4:
		digitalWrite(teclado.columnas[2], LOW);
		digitalWrite(teclado.columnas[3], HIGH);
		break;
	default:
		printf("Número de columna no válido.\n");
		fflush(stdout);
		break;
	}
}

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------

int CompruebaTimeoutColumna (fsm_t* this) {
	TipoTeclado *p_teclado;
	p_teclado = (TipoTeclado*)(this->user_data);
	int result = 0;
	piLock(KEYBOARD_KEY);
	result = p_teclado->flags & FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);
	return result;
}

int CompruebaTeclaPulsada (fsm_t* this) {
	TipoTeclado *p_teclado;
	p_teclado = (TipoTeclado*)(this->user_data);
	int result = 0;
	piLock(KEYBOARD_KEY);
	result = p_teclado->flags & FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);
	return result;
}

//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LAS MAQUINAS DE ESTADOS
//------------------------------------------------------

void TecladoExcitaColumna (fsm_t* this) {
	TipoTeclado *p_teclado;
	p_teclado = (TipoTeclado*)(this->user_data);

	piLock(KEYBOARD_KEY);
	p_teclado->flags &= (~FLAG_TIMEOUT_COLUMNA_TECLADO);
	teclado.columna_actual++; // Pasado el timeout saltamos de columna
	if(p_teclado->columna_actual > COLUMNA_4) teclado.columna_actual = COLUMNA_1;
	ActualizaExcitacionTecladoGPIO(p_teclado->columna_actual);
	piUnlock(KEYBOARD_KEY);

	tmr_startms(p_teclado->tmr_duracion_columna, TIMEOUT_COLUMNA_TECLADO);
}

void ProcesaTeclaPulsada (fsm_t* this) {
	TipoTeclado *p_teclado;
	p_teclado = (TipoTeclado*)(this->user_data);

	piLock(KEYBOARD_KEY);
	p_teclado->flags &= (~FLAG_TECLA_PULSADA);

	switch(p_teclado->teclaPulsada.col){
	case COLUMNA_1:
		if(p_teclado->teclaPulsada.row == FILA_2){ //'4'
			piLock(SYSTEM_FLAGS_KEY);
			flags |= FLAG_MOV_IZQUIERDA;
			piUnlock(SYSTEM_FLAGS_KEY);
			p_teclado->teclaPulsada.row = -1;
			p_teclado->teclaPulsada.col = -1;
		}
	break;
	case COLUMNA_2:
		if(p_teclado->teclaPulsada.row == FILA_2){ //'5'
			piLock(SYSTEM_FLAGS_KEY);
			flags |= FLAG_MOV_DERECHA;
			piUnlock(SYSTEM_FLAGS_KEY);
			p_teclado->teclaPulsada.row = -1;
			p_teclado->teclaPulsada.col = -1;
		}
	break;
	case COLUMNA_4:
		if(p_teclado->teclaPulsada.row == FILA_1){ //'A'
			piLock(SYSTEM_FLAGS_KEY);
			flags |= FLAG_BOTON;
			piUnlock(SYSTEM_FLAGS_KEY);
			p_teclado->teclaPulsada.row = -1;
			p_teclado->teclaPulsada.col = -1;
		}
		if(p_teclado->teclaPulsada.row == FILA_4){ //'D'
			p_teclado->teclaPulsada.row = -1;
			p_teclado->teclaPulsada.col = -1;
			exit(0);
		}
	break;
	default:
		printf("INVALID KEY!!!\n");
		fflush(stdout);
		p_teclado->teclaPulsada.row = -1;
		p_teclado->teclaPulsada.col = -1;
	break;
	}

	piUnlock(KEYBOARD_KEY);
}


//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------

void teclado_fila_1_isr (void) {
	if(millis() < teclado.debounceTime[FILA_1]){
		teclado.debounceTime[FILA_1] = millis() + DEBOUNCE_TIME;
		return;
	}
	piLock(KEYBOARD_KEY);
	teclado.teclaPulsada.row = FILA_1;
	teclado.teclaPulsada.col = teclado.columna_actual;
	teclado.flags |= FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);
	teclado.debounceTime[FILA_1] = millis() + DEBOUNCE_TIME;
}

void teclado_fila_2_isr (void) {
	if(millis() < teclado.debounceTime[FILA_2]){
		teclado.debounceTime[FILA_2] = millis() + DEBOUNCE_TIME;
		return;
	}
	piLock(KEYBOARD_KEY);
	teclado.teclaPulsada.row = FILA_2;
	teclado.teclaPulsada.col = teclado.columna_actual;
	teclado.flags |= FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);
	teclado.debounceTime[FILA_2] = millis() + DEBOUNCE_TIME;
}

void teclado_fila_3_isr (void) {
	if(millis() < teclado.debounceTime[FILA_3]){
		teclado.debounceTime[FILA_3] = millis() + DEBOUNCE_TIME;
		return;
	}
	piLock(KEYBOARD_KEY);
	teclado.teclaPulsada.row = FILA_3;
	teclado.teclaPulsada.col = teclado.columna_actual;
	teclado.flags |= FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);
	teclado.debounceTime[FILA_3] = millis() + DEBOUNCE_TIME;
}

void teclado_fila_4_isr (void) {
	if(millis() < teclado.debounceTime[FILA_4]){
		teclado.debounceTime[FILA_4] = millis() + DEBOUNCE_TIME;
		return;
	}
	piLock(KEYBOARD_KEY);
	teclado.teclaPulsada.row = FILA_4;
	teclado.teclaPulsada.col = teclado.columna_actual;
	teclado.flags |= FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);
	teclado.debounceTime[FILA_4] = millis() + DEBOUNCE_TIME;
}

void timer_duracion_columna_isr (union sigval value) {
	piLock(KEYBOARD_KEY);
	teclado.flags |= FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);
}







