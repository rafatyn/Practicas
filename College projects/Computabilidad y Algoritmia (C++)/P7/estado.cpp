#include "estado.hpp"

estado::estado(void):
transiciones(),
aceptacion(0),
id(-1)
{}

estado::estado(const estado &x)
{
    transiciones = x.get_transiciones();
    aceptacion = x.get_aceptacion();
    id = x.get_id();
}

estado::estado(set <transicion> transiciones_, bool aceptacion_, int id_)
{
    transiciones=transiciones_;
    aceptacion=aceptacion_;
    id=id_;
}

estado::~estado(void)
{}

bool estado::existe(char x) const
{
    for(set <transicion>::iterator i=transiciones.begin(); i!=transiciones.end(); i++){
        if((*i).existe(x)){
            return true;
        }
    }
    return false;
}

void estado::mostrar(void) const
{
    cout << "Estado: " << get_id() << " " << "Aceptacion: " << aceptacion << " " << "Numero transiciones: " << transiciones.size() << " ";
    for(set <transicion>::iterator i=transiciones.begin(); i!=transiciones.end(); i++){
        (*i).mostrar();
    }
}


bool estado::get_aceptacion(void) const
{
    return aceptacion;
}

bool estado::estado_muerte(void) const
{
    int x=0;
    
    for(set <transicion>::iterator i=transiciones.begin(); i!=transiciones.end(); i++){
        if((*i).get_sig_estado()!=id){
            x++;
        }
    }
    if(x!=0||aceptacion==true){
        return false;
    }else{
        return true;
    }
}

int estado::get_id(void) const
{
    return id;
}

set <transicion> estado::get_transiciones(void) const
{
    return transiciones;
}

int estado::estado_siguiente(char x) const
{
    for(set <transicion>::iterator i=transiciones.begin(); i!=transiciones.end(); i++){
        if((*i).existe(x)){
            return (*i).get_sig_estado();
        }
    }
    cout << "Estado siguiente no reconocido" <<endl;
}

set <int> estado::get_conjunto_transiciones(char x) const
{
    set <int> csg;
    for(set <transicion>::iterator i=transiciones.begin(); i!=transiciones.end(); i++){
        if((*i).existe(x)==true){
            csg.insert((*i).get_sig_estado());
        }
    }
    return csg;
}

set <int> estado::get_epsilon_clausura(void) const
{
    set <int> cec;
    for(set <transicion>::iterator i=transiciones.begin(); i!=transiciones.end(); i++){
        if((*i).existe('~')){
            cec.insert((*i).get_sig_estado());
        }
    }
    return cec;
}

bool estado::operator==(const estado &x) const
{
    if(x.get_id()==id){
        return 1;
    }else{
        return 0;
    }
}

estado& estado::operator=(const estado &x)
{
    this->transiciones = x.get_transiciones();
    this->aceptacion = x.get_aceptacion();
    this->id = x.get_id();
    return(*this);
}

int estado::operator<(const estado &x) const
{
    if(id<x.get_id()){
        return 1;
    }else{
        return 0;
    }
}

ostream& operator<<(ostream &output, const estado &x)
{
    output << "Estado: " << x.get_id() << "Aceptacion: " << x.get_aceptacion();
    for(set <transicion>::iterator i=x.get_transiciones().begin(); i!=x.get_transiciones().end(); i++){
        output << (*i);
    }
    return output;
}
    