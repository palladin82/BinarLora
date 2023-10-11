
#ifndef SimpleMenu_h
#define SimpleMenu_h

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#include <wiring.h>
#endif



        
class SimpleMenu
{
    public:
        SimpleMenu(int _numberMenuItems, SimpleMenu *_submenus); //default
        SimpleMenu(void (*_CallBack)());
        SimpleMenu(String _name, void (*_CallbackP)(int*), int);
        SimpleMenu(bool (*_ListCallBack)(int index), void (*_CallBack)());
        SimpleMenu(bool (*_ListCallBack)(int index), SimpleMenu *_submenus);
        SimpleMenu(String _name,int _numberMenuItems, SimpleMenu *_submenus); //sub menu
        SimpleMenu(String _name,int *_value); //Value menu
        SimpleMenu(String _name,int *_value, int _min, int _max); //Value menu with min and max
        SimpleMenu(String _name, uint8_t *_value, int _min, int _max); //Value menu with min and max uint8_t
        SimpleMenu(String _name, void (*_CallBack)()); //function menu
        SimpleMenu(String _name, bool (*_ListCallBack)(int index), SimpleMenu *_submenus); //List menu with function
        SimpleMenu(String _name, bool (*_ListCallBack)(int index), void (*_CallBack)()); //List menu with function

        void begin();
        void begin(void (*_displayCallBack)(SimpleMenu *_menu));
        void begin(void (*_displayCallBack)(SimpleMenu *_menu),void (*_valueCallBack)(SimpleMenu *_menu));
        void begin(SimpleMenu *_Top,void (*_displayCallBack)(SimpleMenu *_menu),void (*_valueCallBack)(SimpleMenu *_menu));
        void setFunctions(void (*_displayCallBack)(SimpleMenu *_menu), void (*_valueCallBack)(SimpleMenu *_menu));

        void home();
        void select();
        void back();
        void returned();
        void up();
        void down();
        void index(int _index);

        SimpleMenu *next();
        SimpleMenu *next(int index);
        
        SimpleMenu *Cur;
        int getValue();
        uint8_t getUValue(); 
        bool hasValue();
        int getIndex();
        void print();
            
        String name;
        bool selectMenu = false;
        int menuLocation = 0;
        int param;
        void (*CallBackP)(int*) = NULL;

    private:
        SimpleMenu *Top_menu = NULL;

        int numberMenuItems;
        SimpleMenu *submenus = NULL;
        SimpleMenu *subListmenus = NULL;
        int *value = NULL;
        
        void (*CallBack) () = NULL;
        
        bool (*ListCallBack)(int index) = NULL;
        void (*displayCallBack)(SimpleMenu *_menu) = NULL;
        void (*valueCallBack)(SimpleMenu *_menu) = NULL;

        int min;
        int max;

        uint8_t *uvalue = NULL;
        uint8_t umin;
        uint8_t umax;
        
        bool minMaxSet = false;
        bool uminMaxSet = false;
};

#endif 
