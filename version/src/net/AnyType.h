#include <iostream>
#include <string>
#include <memory>
#include <typeindex>

class Any{
public:
    Any(void)
        :m_tpIndex(std::type_index(typeid(void))){

    }
    Any(const Any& that)    //左值
        :m_ptr(that.Clone()),
        m_tpIndex(that.m_tpIndex){

    }
    Any(Any&& that)    //右值
        :m_ptr(std::move(that.m_ptr)),
        m_tpIndex(that.m_tpIndex){
    
    }

    template<typename U, class = typename std::enable_if<!std::is_same<typename std::decay<U>::type, Any>::value, U>::type> 
    Any(U&& value):
        m_ptr(new Derived<typename std::decay<U>::type>(std::forward<U>(value))),
        m_tpIndex(std::type_index(typeid(typename std::decay<U>::type))){

    }

    bool isNull()const{
        return !bool(m_ptr);
    }

    template<class U>
    bool is()const{
        return m_tpIndex == std::type_index(typeid(U));
    }

    template<class U>
    U& anyCast(){
        if(!is<U>()){
            throw std::bad_cast();
        }

        auto derived = dynamic_cast<Derived<U>*>(m_ptr.get());
        return derived->m_value;
    }

    Any& operator=(const Any& a){
        if(m_ptr == a.m_ptr){
            return *this;
        }

        m_ptr = a.Clone();
        m_tpIndex = a.m_tpIndex;
        return *this;
    }

private:
    struct Base;
    typedef std::unique_ptr<Base> BasePtr;

    struct Base{
        virtual ~Base(){
        
        }
        virtual BasePtr Clone()const = 0;
    };

    template<typename T>
    struct Derived : Base{
        template<typename U>
        Derived(U&& value)
            :m_value(std::forward<U>(value)){

        }

        BasePtr Clone()const{
            return BasePtr(new Derived<T>(m_value));
        }

        T m_value;
    };

    BasePtr Clone()const{
        if(m_ptr != nullptr){
            return m_ptr->Clone();
        }

        return nullptr;
    }

    BasePtr m_ptr;
    std::type_index m_tpIndex;    //构造type_info只能通过typeid返回对象，type_index是type_info的封装
};
        

