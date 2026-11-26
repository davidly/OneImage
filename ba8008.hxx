// Native Intel 8008 / A08 backend. Included by ba.cxx inside BA_ENABLE_COMPILER.
// DE is the expression accumulator. Hardware calls are shallow runtime helpers;
// BASIC GOSUB return addresses and expression temporaries use separate RAM pages.
// A left operand stays in BC, rather than on the RAM stack, while a "simple"
// right operand is loaded: a constant, scalar, or one-dimensional element with
// a constant or scalar subscript. Those loads use only A, DE and HL.
// Variables are word aligned, so a word's two bytes never straddle a page.
// Scalars normally live in the scratch page at 3F06H; while H is known to hold
// 3FH, a scalar access sets only L. Likewise, while DE is known to hold a
// scalar's value, loading that scalar emits nothing. emit() forgets both;
// keep() preserves H and keepAll() both, for code that changes neither and
// is entered only from the code before it.
#include <stdexcept>
#include <set>

namespace BA8008 {
struct Generator {
    ostringstream code;
    map<string, Variable> const & variables;
    map<string, pair<int,int> > dimensions;
    vector<pair<string,string> > texts;
    map<int,int> forNext, nextFor;
    set<int> targets; // lines entered other than from the previous line
    int serial, currentLine;
    bool scalarPage, hScalar, remainders;
    string deVar; // scalar whose value DE holds, or empty
    // The last statement was q = x / n, so BDIV left x - n*q at 3F04H.
    struct Division { bool valid; TokenValue const * x, * n; string q; } division; // tokens of g_linesOfCode
    Generator(map<string, Variable> const & v) : variables(v), serial(0), currentLine(0),
        scalarPage(false), hScalar(false), remainders(false) { division.valid=false; }
    void emit(string const & s) { code << s << '\n'; hScalar = false; deVar.clear(); }
    void keep(string const & s) { bool h = hScalar; emit(s); hScalar = h; }
    void keepAll(string const & s) { string d = deVar; keep(s); deVar = d; }
    void require(bool b, char const * why) { if (!b) throw runtime_error(why); }
    string unique() { return "BT" + to_string(serial++); }
    string line(int n) { return "BL" + to_string(n); }
    string var(string name) {
        require(variables.count(name) != 0, "unknown variable");
        name.pop_back();
        for (char & c : name) c = (char)toupper((unsigned char)c);
        return "BV_" + name;
    }
    void address(string const & name) { emit(" LHI HIGH " + name + "\n LLI LOW " + name); }
    void scalarAddress(string const & name) {
        if (!scalarPage) { string d = deVar; address(var(name)); deVar = d; return; }
        if (hScalar) keepAll(" LLI LOW " + var(name));
        else { string d = deVar; emit(" LHI 3FH\n LLI LOW " + var(name)); hScalar = true; deVar = d; }
    }
    void constant(int n) {
        unsigned u = (unsigned)n & 65535;
        keep(" LDI " + to_string(u >> 8) + "D\n LEI " + to_string(u & 255) + "D");
    }
    void loadAt() { keep(" LEM\n INL\n LDM"); }
    void storeAt() { keepAll(" LME\n INL\n LMD"); }
    void load(string const & name) { if (deVar != name) { scalarAddress(name); loadAt(); deVar = name; } }
    void store(string const & name) { scalarAddress(name); storeAt(); deVar = name; }
    void addConstant(int n) {
        unsigned u=(unsigned)n&65535;
        if (!u) return;
        if (!(u&255)) { keep(" LAD\n ADI "+to_string(u>>8)+"D\n LDA"); return; }
        keep(" LAE\n ADI "+to_string(u&255)+"D\n LEA\n LAD\n ACI "+to_string(u>>8)+"D\n LDA");
    }
    void shiftLeft(int count) {
        if (count>=8) { keep(" LDE\n LEI 0"); for (count-=8;count>0;--count) keep(" LAD\n ADA\n LDA"); }
        for (;count>0;--count) keep(" LAE\n ADE\n LEA\n LAD\n RAL\n LDA");
    }
    static int bits(unsigned u) { int n=0; for (;u;u>>=1) n+=u&1; return n; }
    static int width(unsigned u) { int n=0; for (;u;u>>=1) ++n; return n; }
    // Shift-and-add multiplication is used for small positive constants.
    static bool shiftAddMultiply(int n) { return n>0 && width((unsigned)n)+bits((unsigned)n)<=14; }
    bool canImmediate(Token t, int n) {
        unsigned u=(unsigned)n&65535;
        if (isOperatorRelational(t) || t==Token_PLUS || t==Token_MINUS || t==Token_AND || t==Token_OR || t==Token_XOR) return true;
        if (t==Token_MULT) return u==0 || !(u&(u-1)) || shiftAddMultiply(n);
        return t==Token_DIV && n>0 && n<=256 && !(n&(n-1));
    }
    void byteOp(Token t, unsigned b, char reg) {
        string r(1,reg);
        if ((t==Token_AND && b==255) || (t!=Token_AND && b==0)) return;
        if (t==Token_AND && b==0) { keep(" L"+r+"I 0"); return; }
        if (t==Token_OR && b==255) { keep(" L"+r+"I 255D"); return; }
        string op=t==Token_AND?"NDI ":(t==Token_OR?"ORI ":"XRI ");
        keep(" LA"+r+"\n "+op+to_string(b)+"D\n L"+r+"A");
    }
    // DE = DE op n. Callers check canImmediate() first; BC is clobbered.
    void immediateOp(Token t, int n) {
        unsigned u=(unsigned)n&65535;
        if (isOperatorRelational(t)) {
            string yes=unique(), done=unique();
            compareConstant(t,n,yes); constant(0); emit(" JMP "+done+"\n"+yes+":");
            constant(1); emit(done+":"); return;
        }
        if (t==Token_PLUS || t==Token_MINUS) { addConstant(t==Token_PLUS?n:-n); return; }
        if (t==Token_AND || t==Token_OR || t==Token_XOR) { byteOp(t,u&255,'E'); byteOp(t,u>>8,'D'); return; }
        if (t==Token_MULT) {
            if (!u) constant(0);
            else if (!(u&(u-1))) shiftLeft(width(u)-1);
            else {
                keepAll(" LBD\n LCE");
                for (int bit=width(u)-2;bit>=0;--bit) {
                    shiftLeft(1);
                    if (u>>bit&1) keep(" LAE\n ADC\n LEA\n LAD\n ACB\n LDA");
                }
            }
            return;
        }
        if (t==Token_DIV) {
            if (n>1) {
                string positive=unique();
                emit(" LAD\n NDA\n JFS "+positive);
                addConstant(n-1); emit(positive+":");
                for (;n>1;n>>=1) emit(" LAD\n RLC\n LAD\n RAR\n LDA\n LAE\n RAR\n LEA");
            }
        }
    }
    bool literal(vector<TokenValue> const & v, int & i, int end, int level, int & n) {
        int cursor=i, sign=1;
        if (cursor<end && v[cursor].token==Token_MINUS) { sign=-1; ++cursor; }
        if (cursor>=end || v[cursor].token!=Token_CONSTANT) return false;
        unsigned value=(unsigned)v[cursor++].value&65535;
        if (sign<0) value=(0-value)&65535;
        if (cursor<end) for (int l=level+1;l<4;++l) if (atLevel(v[cursor].token,l)) return false;
        n=value>=32768?(int)value-65536:(int)value; i=cursor; return true;
    }
    void compareConstant(Token t, int n, string const & dest) {
        unsigned u=(unsigned)n&65535;
        string done=unique();
        keepAll(" LAD");
        if (t!=Token_EQ && t!=Token_NE) keepAll(" XRI 80H");
        keepAll(" CPI "+to_string((u>>8)^((t!=Token_EQ && t!=Token_NE)?128:0))+"D");
        if (t!=Token_EQ && t!=Token_NE) keepAll(" JTC "+((t==Token_LT||t==Token_LE)?dest:done));
        keepAll(" JFZ "+((t==Token_GT||t==Token_GE||t==Token_NE)?dest:done));
        keepAll(" LAE\n CPI "+to_string(u&255)+"D");
        if (t==Token_EQ) keepAll(" JTZ "+dest);
        else if (t==Token_NE) keepAll(" JFZ "+dest);
        else if (t==Token_LT) keepAll(" JTC "+dest);
        else if (t==Token_GE) keepAll(" JFC "+dest);
        else if (t==Token_LE) keepAll(" JTC "+dest+"\n JTZ "+dest);
        else keepAll(" JTC "+done+"\n JFZ "+dest);
        keepAll(done+":");
    }
    static Token inverse(Token t) {
        if (t==Token_EQ) return Token_NE; if (t==Token_NE) return Token_EQ;
        if (t==Token_LT) return Token_GE; if (t==Token_LE) return Token_GT;
        if (t==Token_GT) return Token_LE; return Token_LT;
    }
    static Token mirror(Token t) {
        if (t==Token_LT) return Token_GT; if (t==Token_GT) return Token_LT;
        if (t==Token_LE) return Token_GE; if (t==Token_GE) return Token_LE;
        return t;
    }
    static bool commutative(Token t) {
        return t==Token_PLUS || t==Token_MULT || t==Token_AND || t==Token_OR || t==Token_XOR || t==Token_EQ || t==Token_NE;
    }
    // Jump to dest when BC t DE (signed).
    void jumpRegs(Token t, string const & dest) {
        if (t==Token_EQ) { string skip=unique(); keepAll(" LAC\n CPE\n JFZ "+skip+"\n LAB\n CPD\n JTZ "+dest+"\n"+skip+":"); return; }
        if (t==Token_NE) { keepAll(" LAC\n CPE\n JFZ "+dest+"\n LAB\n CPD\n JFZ "+dest); return; }
        emit(" CAL BCMP");
        string test;
        if (t==Token_LT) test="CPI 1\n JTZ ";
        else if (t==Token_LE) test="CPI 2\n JFZ ";
        else if (t==Token_GT) test="CPI 2\n JTZ ";
        else test="CPI 1\n JFZ ";
        emit(" "+test+dest);
    }
    void compareStack(Token t, string const & dest) { emit(" CAL BPOP"); jumpRegs(t,dest); }
    void increment(string const & name, bool subtract=false) {
        scalarAddress(name);
        keep(string(" LAM\n ")+(subtract?"SUI 1":"ADI 1")+"\n LMA\n LEA\n INL\n LAM\n "+(subtract?"SBI 0":"ACI 0")+"\n LMA\n LDA");
        deVar = name;
    }
    // Top-level tokens of [s,e) satisfying pred, outside parentheses.
    template <class P> vector<int> topLevel(vector<TokenValue> const & v, int s, int e, P pred) {
        vector<int> found; int depth=0;
        for (int k=s;k<e;++k) {
            if (v[k].token==Token_OPENPAREN) ++depth;
            else if (v[k].token==Token_CLOSEPAREN) --depth;
            else if (!depth && pred(v[k].token)) found.push_back(k);
        }
        return found;
    }
    // An expression range that cannot stop with a runtime error, so it may be skipped.
    bool safe(vector<TokenValue> const & v, int s, int e) {
        for (int k=s;k<e;++k) {
            Token t=v[k].token;
            if (t==Token_DIV || t==Token_EXPRESSION) return false;
            if (t==Token_VARIABLE && k+1<e && v[k+1].token==Token_OPENPAREN) {
                if (!dimensions.count(v[k].strValue) || dimensions[v[k].strValue].second) return false;
                if (k+4>=e || v[k+2].token!=Token_EXPRESSION || v[k+2].value!=2 ||
                    v[k+3].token!=Token_CONSTANT || v[k+4].token!=Token_CLOSEPAREN) return false;
                unsigned index=(unsigned)v[k+3].value&65535;
                if (index>=32768 || (int)index>=dimensions[v[k].strValue].first) return false;
                k+=4;
            }
        }
        return true;
    }
    void evaluate(vector<TokenValue> const & v, int s, int e) {
        int k=s; binary(v,k,e,2); require(k==e,"comparison operand not fully consumed");
    }
    // Jump to dest when (X t Y) == sense, for [s,e) holding one relational t.
    void compareJump(vector<TokenValue> const & v, int s, int r, int e, string const & dest, bool sense) {
        Token t=sense?v[r].token:inverse(v[r].token);
        int k=r+1, n=0, after=0;
        if (literal(v,k,e,1,n) && k==e) { evaluate(v,s,r); compareConstant(t,n,dest); return; }
        k=s;
        if (literal(v,k,r,1,n) && k==r) { evaluate(v,r+1,e); compareConstant(mirror(t),n,dest); return; }
        if (simple(v,r+1,e,1,after) && after==e) { evaluate(v,s,r); keepAll(" LBD\n LCE"); evaluate(v,r+1,e); jumpRegs(t,dest); return; }
        if (simple(v,s,r,1,after) && after==r) { evaluate(v,r+1,e); keepAll(" LBD\n LCE"); evaluate(v,s,r); jumpRegs(mirror(t),dest); return; }
        evaluate(v,s,r); emit(" CAL BPUSH"); evaluate(v,r+1,e); compareStack(t,dest);
    }
    // Jump to dest when the IF expression at v[i] is true (sense) or false (!sense).
    void condition(vector<TokenValue> const & v, int & i, string const & dest, bool sense) {
        require(v[i].token==Token_EXPRESSION,"expected IF expression");
        int s=i+1, e=i+v[i].value;
        vector<int> logic=topLevel(v,s,e,[](Token t){return isOperatorLogical(t);});
        vector<int> terms(1,s);
        for (int k:logic) terms.push_back(k+1);
        terms.push_back(e+1);
        bool relations=true, same=true, skippable=true;
        vector<int> relational;
        for (size_t k=0;k+1<terms.size();++k) {
            vector<int> r=topLevel(v,terms[k],terms[k+1]-1,[](Token t){return isOperatorRelational(t);});
            relations=relations && r.size()==1;
            relational.push_back(r.empty()?-1:r[0]);
            if (k) skippable=skippable && safe(v,terms[k],terms[k+1]-1);
        }
        for (int k:logic) same=same && v[k].token==v[logic[0]].token && v[k].token!=Token_XOR;
        if (relations && same && skippable) {
            // AND fails at its first false term; OR succeeds at its first true one.
            bool conjunction=logic.empty() || v[logic[0]].token==Token_AND;
            string skip=unique();
            size_t last=relational.size()-1;
            for (size_t k=0;k<last;++k)
                compareJump(v,terms[k],relational[k],terms[k+1]-1,conjunction==sense?skip:dest,!conjunction);
            compareJump(v,terms[last],relational[last],e,dest,sense);
            emit(skip+":"); i=e; return;
        }
        int n=0, k=0;
        if (logic.size()==1 && v[logic[0]].token==Token_AND && (k=logic[0]+1, literal(v,k,e,0,n)) && k==e) {
            // IF x AND mask: test the masked bytes without forming the result.
            unsigned u=(unsigned)n&65535;
            int j=s; binary(v,j,logic[0],1); require(j==logic[0],"IF operand not fully consumed");
            string skip=unique();
            if (!u) { if (!sense) emit(" JMP "+dest); }
            else if (!(u>>8) || !(u&255)) {
                emit(u&255?" LAE\n NDI "+to_string(u&255)+"D":" LAD\n NDI "+to_string(u>>8)+"D");
                emit((sense?" JFZ ":" JTZ ")+dest);
            } else {
                emit(" LAE\n NDI "+to_string(u&255)+"D\n JFZ "+(sense?dest:skip)+"\n LAD\n NDI "+to_string(u>>8)+"D");
                emit((sense?" JFZ ":" JTZ ")+dest);
            }
            emit(skip+":"); i=e; return;
        }
        expression(v,i);
        emit(string(" LAD\n ORE\n ")+(sense?"JFZ ":"JTZ ")+dest);
    }
    void text(string const & value) {
        string label = unique();
        texts.push_back(make_pair(label, value));
        address(label); emit(" CAL BSTR");
    }
    // DE = BC t DE.
    void opRegs(Token t) {
        static char const * names[] = {"BMUL","BDIV","","","BEQ","BNE",
            "BLE","BGE","BLT","BGT"};
        require(t >= Token_MULT && t <= Token_XOR, "unsupported operator");
        if (t==Token_PLUS) keep(" LAE\n ADC\n LEA\n LAD\n ACB\n LDA");
        else if (t==Token_MINUS) keep(" LAC\n SUE\n LEA\n LAB\n SBD\n LDA");
        else if (t==Token_AND || t==Token_OR || t==Token_XOR) {
            string o=t==Token_AND?"ND":(t==Token_OR?"OR":"XR");
            keep(" LAE\n "+o+"C\n LEA\n LAD\n "+o+"B\n LDA");
        } else emit(string(" CAL ") + names[t - Token_MULT]);
    }
    // DE = DE t BC.
    void opSwapped(Token t) {
        if (t==Token_MINUS) keep(" LAE\n SUC\n LEA\n LAD\n SBB\n LDA");
        else if (t==Token_DIV) { emit(" LAB\n LBD\n LDA\n LAC\n LCE\n LEA"); opRegs(t); }
        else opRegs(commutative(t)?t:mirror(t));
    }
    void op(Token t) { emit(" CAL BPOP"); opRegs(t); }
    void expression(vector<TokenValue> const & v, int & i) {
        require(i < (int)v.size() && v[i].token == Token_EXPRESSION, "expected expression");
        int end = i + v[i].value;
        ++i; binary(v, i, end, 0);
        require(i == end, "expression not fully consumed");
    }
    bool atLevel(Token t, int level) {
        if (level == 0) return isOperatorLogical(t);
        if (level == 1) return isOperatorRelational(t);
        if (level == 2) return isOperatorAdditive(t);
        return isOperatorMultiplicative(t);
    }
    // A constant, scalar or 1-D element with constant/scalar subscript that
    // completes the operand at this precedence level; loading it keeps BC.
    bool simple(vector<TokenValue> const & v, int i, int end, int level, int & after) {
        int c=i, n=0;
        if (literal(v,c,end,level,n)) { after=c; return true; }
        if (c>=end || v[c].token!=Token_VARIABLE) return false;
        if (++c<end && v[c].token==Token_OPENPAREN) {
            auto d=dimensions.find(v[i].strValue);
            if (d==dimensions.end() || d->second.second || c+3>=end || v[c+1].token!=Token_EXPRESSION ||
                v[c+1].value!=2 || (v[c+2].token!=Token_CONSTANT && v[c+2].token!=Token_VARIABLE) ||
                v[c+3].token!=Token_CLOSEPAREN) return false;
            c+=4;
        }
        if (c<end) for (int l=level+1;l<4;++l) if (atLevel(v[c].token,l)) return false;
        after=c; return true;
    }
    void operand(vector<TokenValue> const & v, int & i, int end, int level) {
        int n=0;
        if (literal(v,i,end,level,n)) constant(n); else factor(v,i,end);
    }
    void binary(vector<TokenValue> const & v, int & i, int end, int level) {
        if (level == 4) { factor(v, i, end); return; }
        int first=i, after=0, n=0, k=i;
        bool done=false;
        if (literal(v,k,end,level,n) && k<end && atLevel(v[k].token,level) &&
            (commutative(v[k].token) || isOperatorRelational(v[k].token)) && canImmediate(mirror(v[k].token),n)) {
            // Constant first: evaluate the other operand, then use the immediate form.
            Token t=mirror(v[k].token);
            i=k+1; binary(v,i,end,level+1); immediateOp(t,n); done=true;
        } else if (simple(v,first,end,level,after) && after<end && atLevel(v[after].token,level)) {
            int r=after+1, rightEnd=0; k=r;
            Token t=v[after].token;
            if (!(literal(v,k,end,level,n) && canImmediate(t,n)) && !simple(v,r,end,level,rightEnd)) {
                // Simple first operand, complex second: evaluate the second first.
                i=r; binary(v,i,end,level+1);
                keepAll(" LBD\n LCE");
                k=first; operand(v,k,end,level); opSwapped(t); done=true;
            }
        }
        if (!done) binary(v, i, end, level + 1);
        while (i < end && atLevel(v[i].token, level)) {
            Token t = v[i++].token;
            int next=i;
            if (literal(v,next,end,level,n) && canImmediate(t,n)) { immediateOp(t,n); i=next; continue; }
            if (simple(v,i,end,level,after)) { keepAll(" LBD\n LCE"); operand(v,i,end,level); opRegs(t); continue; }
            emit(" CAL BPUSH");
            binary(v, i, end, level + 1);
            op(t);
        }
    }
    void arrayAddress(vector<TokenValue> const & v, int & i, string const & name) {
        require(v[i++].token == Token_OPENPAREN, "expected array subscript");
        require(dimensions.count(name) != 0, "array used without DIM");
        int count = dimensions[name].first * (dimensions[name].second ? dimensions[name].second : 1);
        int cursor=i, value=0;
        if (v[cursor].token==Token_EXPRESSION && v[cursor].value==2 && v[cursor+1].token==Token_CONSTANT && v[cursor+2].token==Token_CLOSEPAREN) {
            value=(int)((unsigned)v[cursor+1].value&65535);
            if (value>=32768 || value>=count) { emit(" JMP BERROR"); value=0; }
            address(var(name)+"+"+to_string(2*value)+"D"); i=cursor+3; return;
        }
        expression(v, i);
        if (v[i].token == Token_COMMA) {
            require(dimensions[name].second != 0, "two subscripts on one-dimensional array");
            emit(" CAL BPUSH"); constant(dimensions[name].second); op(Token_MULT);
            emit(" CAL BPUSH"); ++i; expression(v, i); op(Token_PLUS);
        }
        require(v[i++].token == Token_CLOSEPAREN, "expected close parenthesis");
        string checked=unique();
        emit(" LAD\n CPI "+to_string(count>>8)+"D\n JTC "+checked+"\n JFZ BERROR\n LAE\n CPI "+to_string(count&255)+"D\n JFC BERROR\n"+checked+":");
        emit(" LAE\n ADE\n LEA\n LAD\n ACD\n LDA");
        emit(" LAE\n ADI LOW " + var(name) + "\n LLA\n LAD\n ACI HIGH " + var(name) + "\n LHA");
    }
    void factor(vector<TokenValue> const & v, int & i, int end) {
        require(i < end, "missing expression operand");
        TokenValue const & tv = v[i++];
        if (tv.token == Token_CONSTANT) constant(tv.value);
        else if (tv.token == Token_VARIABLE) {
            if (i < end && v[i].token == Token_OPENPAREN) {
                arrayAddress(v, i, tv.strValue); loadAt();
            } else load(tv.strValue);
        } else if (tv.token == Token_OPENPAREN) {
            if (v[i].token == Token_EXPRESSION) expression(v, i);
            else binary(v, i, end, 0);
            require(v[i++].token == Token_CLOSEPAREN, "missing close parenthesis");
        } else if (tv.token == Token_MINUS || tv.token == Token_NOT) {
            factor(v, i, end);
            emit(tv.token == Token_MINUS ? " CAL BNEG" : " CAL BNOT");
        } else throw runtime_error("unsupported expression token");
    }
    static bool same(TokenValue const & a, TokenValue const & b) {
        return a.token==b.token && (a.token==Token_CONSTANT ? a.value==b.value : a.strValue==b.strValue);
    }
    bool scalar(vector<TokenValue> const & v, int k, int end) {
        return v[k].token==Token_VARIABLE && !dimensions.count(v[k].strValue) && (k+1>=end || v[k+1].token!=Token_OPENPAREN);
    }
    // q = x / n with scalar q and x, and a scalar or constant n that BDIV divides by.
    void noteDivision(vector<TokenValue> const & v, int k, int end, string const & q) {
        if (v[k].token!=Token_EXPRESSION || v[k].value!=4 || k+4!=end || v[k+2].token!=Token_DIV) return;
        TokenValue const & x=v[k+1], & n=v[k+3];
        if (!scalar(v,k+1,end) || x.strValue==q) return;
        if (n.token==Token_CONSTANT ? canImmediate(Token_DIV,n.value) : !scalar(v,k+3,end) || n.strValue==q) return;
        division.valid=true; division.x=&x; division.n=&n; division.q=q;
    }
    // x - n*q, x - q*n or their parenthesized forms, just after a matching division.
    bool remainder(vector<TokenValue> const & v, int k, Division const & d) {
        if (!d.valid || v[k].token!=Token_EXPRESSION) return false;
        int e=k+v[k].value, c=k+1;
        if (e-c!=5 && e-c!=7) return false;
        if (!same(v[c],*d.x) || v[c+1].token!=Token_MINUS) return false;
        c+=2;
        bool parens=e-k==8;
        if (parens && (v[c].token!=Token_OPENPAREN || v[e-1].token!=Token_CLOSEPAREN)) return false;
        if (parens) ++c;
        if (v[c+1].token!=Token_MULT) return false;
        TokenValue q=v[c]; q.token=Token_VARIABLE; q.strValue=d.q;
        if (!((same(v[c],*d.n) && same(v[c+2],q)) || (same(v[c],q) && same(v[c+2],*d.n)))) return false;
        remainders=true;
        emit(" LHI 3FH\n LLI 4\n LEM\n INL\n LDM");
        hScalar=true;
        return true;
    }
    void value(vector<TokenValue> const & v, int & i, Division const & d) {
        if (remainder(v,i,d)) i+=v[i].value; else expression(v,i);
    }
    void statements(vector<TokenValue> const & v, int i, int end) {
        while (i < end) {
            Division previous=division;
            division.valid=false;
            TokenValue const & tv = v[i++];
            if (tv.token == Token_VARIABLE) {
                bool array = v[i].token == Token_OPENPAREN;
                int after = 0;
                if (array && simple(v, i - 1, end, 3, after)) {
                    // Value first, then the address; the subscript load keeps BC.
                    int subscript = i;
                    i = after;
                    require(v[i++].token == Token_EQ, "assignment missing equals");
                    value(v, i, previous);
                    keepAll(" LBD\n LCE");
                    arrayAddress(v, subscript, tv.strValue);
                    emit(" LMC\n INL\n LMB");
                    continue;
                }
                if (array) { arrayAddress(v, i, tv.strValue); emit(" LDH\n LEL\n CAL BPUSH"); }
                require(v[i++].token == Token_EQ, "assignment missing equals");
                int rhs = i;
                if (array) expression(v, i); else value(v, i, previous); // a subscript may divide
                if (array) { emit(" CAL BPOP\n LHB\n LLC"); storeAt(); }
                else { store(tv.strValue); noteDivision(v, rhs, i, tv.strValue); }
            } else if (tv.token == Token_ATOMIC) {
                TokenValue const & a = v[i++];
                increment(a.strValue,a.token!=Token_INC);
            } else if (tv.token == Token_IF) {
                int then = i + v[i].value;
                require(then < end && v[then].token == Token_THEN, "missing THEN token");
                if (!v[then].value && then + 2 == end &&
                    (v[then + 1].token == Token_GOTO || v[then + 1].token == Token_RETURN)) {
                    // IF ... THEN GOTO/RETURN branches straight to the target.
                    condition(v, i, v[then + 1].token == Token_GOTO ? line(v[then + 1].value) : "BRRETURN", true);
                    return;
                }
                string no = unique(), done = unique();
                condition(v,i,no,false);
                require(v[i].token == Token_THEN, "missing THEN token");
                int alternative = v[i].value ? i + v[i].value : end;
                statements(v, i + 1, alternative);
                emit(" JMP " + done + "\n" + no + ":");
                division.valid = false;
                if (alternative < end) statements(v, alternative + 1, end);
                emit(done + ":"); division.valid = false; return;
            } else if (tv.token == Token_GOTO) emit(" JMP " + line(tv.value));
            else if (tv.token == Token_GOSUB) {
                string ret = line(currentLine + 1);
                emit(" LDI HIGH " + ret + "\n LEI LOW " + ret + "\n CAL BRPUSH\n JMP " + line(tv.value));
            } else if (tv.token == Token_RETURN) emit(" JMP BRRETURN");
            else if (tv.token == Token_END) emit(" JMP BEXIT");
            else if (tv.token == Token_PRINT) {
                while (i < end) {
                    if (v[i].token == Token_SEMICOLON) { ++i; continue; }
                    if (v[i].token != Token_EXPRESSION) break;
                    int next = i + v[i].value;
                    TokenValue const & item = v[i + 1];
                    if (item.token == Token_STRING) text(item.strValue);
                    else if (item.token == Token_TIME) text("00:00:00.000");
                    else if (item.token == Token_ELAP) text("0");
                    else { expression(v, i); emit(" CAL BNUMBER"); }
                    i = next;
                }
                if (!tv.extra) emit(" LAI 10D\n OUT 12");
            } else if (tv.token == Token_FOR) {
                require(forNext.count(currentLine) != 0, "FOR without NEXT");
                expression(v, i); store(tv.strValue);
                keepAll("BF" + to_string(currentLine) + ":"); // never a jump target
                load(tv.strValue);
                emit("BFD" + to_string(currentLine) + ":"); // NEXT arrives with the counter in DE
                deVar = tv.strValue;
                int after=0;
                if (v[i].value==2 && v[i+1].token==Token_CONSTANT) {
                    compareConstant(Token_GT,v[i+1].value,line(forNext[currentLine]+1)); i+=2;
                } else if (simple(v,i+1,i+v[i].value,0,after) && after==i+v[i].value) {
                    keepAll(" LBD\n LCE"); ++i; operand(v,i,after,0); jumpRegs(Token_GT,line(forNext[currentLine]+1));
                } else {
                    emit(" CAL BPUSH"); expression(v,i); compareStack(Token_GT,line(forNext[currentLine]+1));
                }
            } else if (tv.token == Token_NEXT) {
                require(nextFor.count(currentLine) != 0, "NEXT without FOR");
                load(tv.strValue);
                // The next signed value is unrepresentable after 32767.
                compareConstant(Token_EQ,32767,line(currentLine+1));
                addConstant(1); store(tv.strValue);
                emit(" JMP BFD" + to_string(nextFor[currentLine]));
            } else if (tv.token != Token_DIM && tv.token != Token_REM &&
                       tv.token != Token_TRON && tv.token != Token_TROFF && tv.token != Token_COLON)
                throw runtime_error("unsupported statement token");
        }
    }
    void runtime() {
        emit(R"BA(
; Runtime ABI: DE=value (right operand), BC=left operand. HL/A are scratch.
; 127 words per checked stack. No BASIC call holds a hardware return slot.
BPUSH: LHI 3FH
 LLI 0
 LAM
 CPI 254D
 JTZ BERROR
 LLA
 ADI 2
 LHI 3FH
 LLI 0
 LMA
 SUI 2
 LLA
 LHI 3CH
 LME
 INL
 LMD
 RET
BPOP: LHI 3FH
 LLI 0
 LAM
 NDA
 JTZ BERROR
 SUI 2
 LMA
 LLA
 LHI 3CH
 LCM
 INL
 LBM
 RET
BRPUSH: LHI 3FH
 LLI 1
 LAM
 CPI 254D
 JTZ BERROR
 ADI 2
 LMA
 SUI 2
 LLA
 LHI 3DH
 LME
 INL
 LMD
 RET
BRRETURN: LHI 3FH
 LLI 1
 LAM
 NDA
 JTZ BERROR
 SUI 2
 LMA
 LLA
 LHI 3DH
 LCM
 INL
 LBM
 LHI HIGH (BRJUMP+1)
 LLI LOW (BRJUMP+1)
 LMC
 INL
 JFZ BRPATCH
 INH
BRPATCH:
 LMB
BRJUMP: JMP 0
BNEG: LAI 0
 SUE
 LEA
 LAI 0
 SBD
 LDA
 RET
BMUL: LHI 0
 LLI 0
BMLOOP: LAD
 ORE
 JTZ BMDONE
 LAE
 NDI 1
 JTZ BMSHIFT
 LAL
 ADC
 LLA
 LAH
 ACB
 LHA
BMSHIFT: LAC
 ADC
 LCA
 LAB
 ACB
 LBA
 LAD
 NDA
 RAR
 LDA
 LAE
 RAR
 LEA
 JMP BMLOOP
BMDONE: LDH
 LEL
 RET
BDIV: LHB
 LLC
 LBD
 LCE
 LDH
 LEL
 LAB
 ORC
 JTZ BERROR
 LAB
 XRD
 LHI 3FH
 LLI 2
 LMA
 INL
 LMD
 LAD
 NDA
 CTS BNEG
)BA");
        emit(R"BA(
 LAB
 NDA
 JFS BDPOS
 LAI 0
 SUC
 LCA
 LAI 0
 SBB
 LBA
BDPOS: LHI 0
 LLI 0
BDLOOP: LAE
 SUC
 LEA
 LAD
 SBB
 LDA
 JTC BDDONE
 INL
 JFZ BDLOOP
 INH
 JMP BDLOOP
BDDONE:)BA");
        if (remainders) emit(R"BA( LAE
 ADC
 LEA
 LAD
 ACB
 LDA
 LBH
 LCL
 LHI 3FH
 LLI 3
 LAM
 NDA
 CTS BNEG
 INL
 LME
 INL
 LMD
 LDB
 LEC)BA");
        else emit(" LDH\n LEL");
        emit(R"BA( LHI 3FH
 LLI 2
 LAM
 NDA
 CTS BNEG
 RET
; Signed comparison returns A=0 equal, 1 less, 2 greater.
BCMP: LAB
 XRD
 JTS BCSIGN
 LAB
 CPD
 JTC BCLESS
 JFZ BCGREATER
 LAC
 CPE
 JTC BCLESS
 JFZ BCGREATER
 XRA
 RET
BCSIGN: LAB
 NDA
 JTS BCLESS
BCGREATER: LAI 2
 RET
BCLESS: LAI 1
 RET
)BA");
        char const * labels[] = {"BEQ", "BNE", "BLE", "BGE", "BLT", "BGT"};
        char const * tests[] = {"CPI 0\n JTZ BTRUE", "CPI 0\n JFZ BTRUE",
            "CPI 2\n JFZ BTRUE", "CPI 1\n JFZ BTRUE", "CPI 1\n JTZ BTRUE", "CPI 2\n JTZ BTRUE"};
        for (int i=0;i<6;++i) emit(string(labels[i])+": CAL BCMP\n "+tests[i]+"\n JMP BFALSE");
        emit(R"BA(
BNOT: LAD
 ORE
 JTZ BTRUE
BFALSE: LDI 0
 LEI 0
 RET
BTRUE: LDI 0
 LEI 1
 RET
BSTR: LAM
 NDA
 RTZ
 OUT 12
 INL
 JFZ BSTR
 INH
 JMP BSTR
BNUMBER: LAD
 NDA
 JFS BUNSIGNED
 LAI 45D
 OUT 12
 CAL BNEG
BUNSIGNED: LLI 0
 LBI HIGH 10000D
 LCI LOW 10000D
 CAL BDIGIT
 LBI HIGH 1000D
 LCI LOW 1000D
 CAL BDIGIT
 LBI 0
 LCI 100D
 CAL BDIGIT
 LCI 10D
 CAL BDIGIT
 LCI 1
 LLI 1
 CAL BDIGIT
 RET
BDIGIT: LHI 0
BDSUB: LAE
 SUC
 LEA
 LAD
 SBB
 LDA
 JTC BDRESTORE
 INH
 JMP BDSUB
BDRESTORE: LAE
 ADC
 LEA
 LAD
 ACB
 LDA
 LAH
 NDA
 JFZ BDSHOW
 LAL
 NDA
 RTZ
BDSHOW: LAH
 ADI 48D
 OUT 12
 LLI 1
 RET
BERROR: LAI 33D
 OUT 12
 LAI 10D
 OUT 12
 HLT
)BA");
    }
    string generate() {
        vector<int> loops;
        for (int l=0;l<(int)g_linesOfCode.size();++l) {
            vector<TokenValue> const & v=g_linesOfCode[l].tokenValues;
            for (TokenValue const & t : v) if(t.token==Token_DIM) {
                require(!dimensions.count(t.strValue), "repeated DIM is not supported by 8008 target");
                dimensions[t.strValue]=make_pair(t.dims[0],t.dimensions==2?t.dims[1]:0);
            }
            if(v[0].token==Token_FOR) loops.push_back(l);
            if(v[0].token==Token_NEXT) {
                require(!loops.empty(),"NEXT without FOR");
                int f=loops.back(); loops.pop_back();
                require(g_linesOfCode[f].tokenValues[0].strValue==v[0].strValue,"NEXT variable mismatch");
                forNext[f]=l; nextFor[l]=f;
            }
        }
        require(loops.empty(),"FOR without NEXT");
        for (int l=0;l<(int)g_linesOfCode.size();++l)
            for (TokenValue const & t : g_linesOfCode[l].tokenValues) {
                if (t.token==Token_GOTO || t.token==Token_GOSUB) targets.insert(t.value);
                if (t.token==Token_GOSUB) targets.insert(l+1); // return point
            }
        for (auto const & f : forNext) targets.insert(f.second+1); // loop exit
        // 3F00H-3F05H hold stack counters, division signs and the remainder.
        int scalars=0;
        for (auto const & v:variables) scalars+=!dimensions.count(v.first);
        scalarPage=scalars<=(256-6)/2;
        long long bytes=0;
        for(auto const & v:variables) {
            auto d=dimensions.find(v.first);
            if (d==dimensions.end()) bytes += scalarPage?0:2;
            else bytes += 2LL*d->second.first*(d->second.second?d->second.second:1);
        }
        require(bytes<0x3c00-64,"8008 data alone exceeds 16 KiB RAM (two-byte array elements)");
        emit("; BA Intel 8008 native code. Assemble with A08; run with ntv8008.\n; Signed 16-bit integers. TIME$/ELAP$ are fixed placeholders (no clock device).\n ORG 0040H\n LHI 3FH\n LLI 0\n LMI 0\n INL\n LMI 0");
        if(!g_Quiet) text("running basic\n");
        for(currentLine=0;currentLine<(int)g_linesOfCode.size();++currentLine) {
            // Only the previous line falls into a line that is not a jump target.
            string label=line(currentLine)+": ; BASIC "+to_string(g_linesOfCode[currentLine].lineNumber);
            if (targets.count(currentLine)) { division.valid=false; emit(label); } else keepAll(label);
            auto const & v=g_linesOfCode[currentLine].tokenValues;
            statements(v,0,(int)v.size());
        }
        emit(line(currentLine)+":\nBEXIT:");
        if(!g_Quiet) text("done running basic\n");
        emit(" HLT"); runtime();
        for(auto const & item:texts) {
            emit(item.first+":");
            for(unsigned char c:item.second) emit(" DATA "+to_string(c)+"D");
            emit(" DATA 0");
        }
        emit(" DS ($ AND 1) ; word-align variables"); // see loadAt()/storeAt()
        for (int pass=0;pass<2;++pass) {
            if (pass && scalarPage) emit(" ORG 3F06H ; scalars, after the image's size count");
            for(auto const & v:variables) {
                auto d=dimensions.find(v.first);
                if ((d==dimensions.end()) != (pass==1)) continue;
                emit(var(v.first)+":");
                if(d==dimensions.end()) {
                    unsigned value=(unsigned)v.second.value&65535;
                    emit(" DATA "+to_string(value&255)+"D,"+to_string(value>>8)+"D");
                } else emit(" DS "+to_string(2*d->second.first*(d->second.second?d->second.second:1))+"D");
            }
        }
        // Count exact instruction/data lengths, not source lines. Fixed stacks
        // begin at 0x3c00; never let code, literals or arrays overlap them.
        istringstream input(code.str()); string s; int pc=0;
        bool origin=false;
        while(getline(input,s)) {
            s=s.substr(0,s.find(';'));
            size_t colon=s.find(':'); if(colon!=string::npos) s=s.substr(colon+1);
            istringstream fields(s); string op,arg; fields>>op;
            if(op.empty()) continue;
            if(op=="ORG") { if (origin) break; origin=true; pc=64; continue; }
            if(op=="DS") { fields>>arg; pc+='('==arg[0]?1:atoi(arg.c_str()); }
            else if(op=="DATA") { fields>>arg; pc+=1+(int)count(arg.begin(),arg.end(),','); }
            else if(op=="CAL" || op=="JMP" || (op[0]=='J') || (op[0]=='C'&&op.size()==3&&op[1]!='P')) pc+=3;
            else if(op=="LAI" || op=="LBI" || op=="LCI" || op=="LDI" || op=="LEI" || op=="LHI" || op=="LLI" || op=="LMI" || op=="ADI" || op=="ACI" || op=="SUI" || op=="SBI" || op=="CPI" || op=="NDI" || op=="ORI" || op=="XRI") pc+=2;
            else ++pc;
        }
        require(pc<=0x3c00,"8008 code and data exceed available RAM below software stacks");
        emit("; End of image: "+to_string(pc)+" bytes; stacks 3C00H/3D00H, scratch "+(scalarPage?"and scalars ":"")+"3F00H.\n END");
        return code.str();
    }
};
} // namespace BA8008

void Generate8008(char const * path, map<string, Variable> const & variables)
{
    try {
        BA8008::Generator generator(variables);
        string source=generator.generate();
        CFile output(fopen(path,"w"));
        if(!output.get()) throw runtime_error("cannot open 8008 output file");
        if(fwrite(source.data(),1,source.size(),output.get())!=source.size()) throw runtime_error("cannot write 8008 output file");
    } catch(exception const & e) {
        fprintf(stderr,"BA 8008: %s\n",e.what());
        exit(1);
    }
}
