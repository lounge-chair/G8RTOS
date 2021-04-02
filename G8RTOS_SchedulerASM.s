; G8RTOS_SchedulerASM.s
; Holds all ASM functions needed for the scheduler
; Note: If you have an h file, do not have a C file and an S file of the same name

	; Functions Defined
	.def G8RTOS_Start, PendSV_Handler

	; Dependencies
	.ref CurrentlyRunningThread, G8RTOS_Scheduler

	.thumb		; Set to thumb mode
	.align 2	; Align by 2 bytes (thumb mode uses allignment by 2 or 4)
	.text		; Text section

; Need to have the address defined in file 
; (label needs to be close enough to asm code to be reached with PC relative addressing)
RunningPtr: .field CurrentlyRunningThread, 32

; G8RTOS_Start
;	Sets the first thread to be the currently running thread
;	Starts the currently running thread by setting Link Register to tcb's Program Counter
G8RTOS_Start:

	.asmfunc
	CPSID I					; interrupt disable

	LDR R0, RunningPtr		; R0 has address of RunningPtr
	LDR R1, [R0]			; R1 has value within running ptr -- a ptr to the thread's stack
	LDR SP, [R1]			; load stack pointer from tcb

	POP {R4 - R11}			; load fake context into registers
	POP {R0 - R3}
	POP {R12}
	ADD SP, SP, #4			; discard LR from initial stack
	POP {LR}				; set link register to tcb program counter
	ADD SP, SP, #4			; discard psr
	CPSIE I
	BX LR
	.endasmfunc

; PendSV_Handler
; - Performs a context switch in G8RTOS
; 	- Saves remaining registers into thread stack
;	- Saves current stack pointer to tcb
;	- Calls G8RTOS_Scheduler to get new tcb
;	- Set stack pointer to new stack pointer from new tcb
;	- Pops registers from thread stack
PendSV_Handler:
	
	.asmfunc

	CPSID I				; disable interrupts (enter critical section)

	PUSH {R4 - R11}		; saves remaining registers into thread stack

	LDR R0, RunningPtr  ; R0 has address of RunningPtr
	LDR R1, [R0]		; R1 has value within running ptr -- a ptr to the thread's stack
	STR SP, [R1]		; saves current stack pointer to tcb

	PUSH {R0, LR}		; preserve values of registers
	BL G8RTOS_Scheduler	; calls G8RTOS_Scheduler to get new tcb
	POP	 {R0, LR}		; restore values of registers

	LDR R1, [R0]		; set stack pointer to new stack pointer from new TCB
	LDR SP, [R1]

	POP {R4 - R11}

	CPSIE I 			; re-enable interrupts (leave critical section)

	BX LR

	.endasmfunc
	
	; end of the asm file
	.align
	.end
