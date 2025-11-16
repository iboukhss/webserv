#ifndef HANDLER_HANDLER_HPP_
#define HANDLER_HANDLER_HPP_

class Handler {
public:
    virtual ~Handler() {}

    virtual int read_data(char* buf, int n) = 0;
    virtual int write_data(const char* buf, int n) = 0;

    virtual bool is_readable() const = 0;
    virtual bool is_writable() const = 0;
    virtual bool is_done() const = 0;

protected:
    Handler() {} // I read the docs, apparently this is legit

private:
    Handler(const Handler&);
    Handler& operator=(const Handler&);
};

#endif
