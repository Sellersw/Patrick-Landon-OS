/*


*/

#include "../e/initial.e"

void sysCallHandler(){
  unsigned int call, status;

  state_t *oldState = (state_t *) SYSCALLOLD;
  status = oldState->s_status;
  call = oldState->s_a0;

  if(call >= 9){
    // Pass up or die
  }



  switch(call){
    case 1:

    case 2:

    case 3:

    case 4:

    case 5:

    case 6:

    case 7:

    case 8:

  }
}

void progTrapHandler(){

}

void tlbTrapHandler(){

}
