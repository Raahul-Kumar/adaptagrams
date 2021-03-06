/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2005-2012  Monash University
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * See the file LICENSE.LGPL distributed with the library.
 *
 * Licensees holding a valid commercial license may use this file in
 * accordance with the commercial license agreement provided with the 
 * library.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *
 * Author(s):   Tim Dwyer  <Tim.Dwyer@csse.monash.edu.au>
 *              Michael Wybrow  <mjwybrow@users.sourceforge.net>
 *
 * --------------
 *
 * This file contains a slightly modified version of IncSolver() from libvpsc:
 * A solver for the problem of Variable Placement with Separation Constraints.
 * It has the following changes from the Adaptagrams VPSC version:
 *  -  The required VPSC code has been consolidated into a single file.
 *  -  Unnecessary code (like Solver) has been removed.
 *  -  The PairingHeap code has been replaced by a STL priority_queue.
 *
 * Modifications:  Michael Wybrow  <mjwybrow@users.sourceforge.net>
 *
*/

#ifndef LIBAVOID_VPSC_H
#define LIBAVOID_VPSC_H


#ifdef USELIBVPSC

// By default, libavoid will use it's own version of VPSC defined in this file.
//
// Alternatively, you can directly use IncSolver from libvpsc.  This
// introduces a dependency on libvpsc but it can be preferable in cases 
// where you are building all of Adaptagrams together and want to work
// with a set of CompoundConstraints or other classes built upon the 
// base libvpsc Constraint classes.

// Include necessary headers from libvpsc.
#include "libvpsc/variable.h"
#include "libvpsc/constraint.h"
#include "libvpsc/rectangle.h"
#include "libvpsc/solve_VPSC.h"

// Use the libvpsc versions of things needed by libavoid.
using vpsc::Variable;
using vpsc::Variables;
using vpsc::Constraint;
using vpsc::Constraints;
using vpsc::IncSolver;
using vpsc::delete_object;

#else

#include <vector>
#include <list>
#include <set>
#include <queue>
#include <iostream>

namespace Avoid {

class Variable;
class Constraint;
class Blocks;
typedef std::vector<Variable*> Variables;
typedef std::vector<Constraint*> Constraints;
class CompareConstraints {
public:
    bool operator() (Constraint *const &l, Constraint *const &r) const;
};
struct PositionStats {
    PositionStats() : scale(0), AB(0), AD(0), A2(0) {}
    void addVariable(Variable* const v);
    double scale;
    double AB;
    double AD;
    double A2;
};

typedef std::priority_queue<Constraint*,std::vector<Constraint*>,
        CompareConstraints> Heap;

class Block
{
    typedef Variables::iterator Vit;
    typedef Constraints::iterator Cit;
    typedef Constraints::const_iterator Cit_const;

    friend std::ostream& operator <<(std::ostream &os,const Block &b);
public:
    Variables *vars;
    double posn;
    //double weight;
    //double wposn;
    PositionStats ps;
    Block(Blocks *blocks, Variable* const v=NULL);
    ~Block(void);
    Constraint* findMinLM();
    Constraint* findMinLMBetween(Variable* const lv, Variable* const rv);
    Constraint* findMinInConstraint();
    Constraint* findMinOutConstraint();
    void deleteMinInConstraint();
    void deleteMinOutConstraint();
    void updateWeightedPosition();
    void merge(Block *b, Constraint *c, double dist);
    Block* merge(Block *b, Constraint *c);
    void mergeIn(Block *b);
    void mergeOut(Block *b);
    void split(Block *&l, Block *&r, Constraint *c);
    Constraint* splitBetween(Variable* vl, Variable* vr, Block* &lb, Block* &rb);
    void setUpInConstraints();
    void setUpOutConstraints();
    double cost();
    bool deleted;
    long timeStamp;
    Heap *in;
    Heap *out;
    bool getActivePathBetween(Constraints& path, Variable const* u,
               Variable const* v, Variable const *w) const;
    bool isActiveDirectedPathBetween(
            Variable const* u, Variable const* v) const;
    bool getActiveDirectedPathBetween(Constraints& path, Variable const * u, Variable const * v) const;
private:
    typedef enum {NONE, LEFT, RIGHT} Direction;
    typedef std::pair<double, Constraint*> Pair;
    void reset_active_lm(Variable* const v, Variable* const u);
    void list_active(Variable* const v, Variable* const u);
    double compute_dfdv(Variable* const v, Variable* const u);
    double compute_dfdv(Variable* const v, Variable* const u, Constraint *&min_lm);
    bool split_path(Variable*, Variable* const, Variable* const, 
            Constraint* &min_lm, bool desperation);
    bool canFollowLeft(Constraint const* c, Variable const* last) const;
    bool canFollowRight(Constraint const* c, Variable const* last) const;
    void populateSplitBlock(Block *b, Variable* v, Variable const* u);
    void addVariable(Variable* v);
    void setUpConstraintHeap(Heap* &h,bool in);

    // Parent container, that holds the blockTimeCtr.
    Blocks *blocks;
};


class Constraint;
typedef std::vector<Constraint*> Constraints;
class Variable
{
    friend std::ostream& operator <<(std::ostream &os, const Variable &v);
    friend class Block;
    friend class Constraint;
    friend class IncSolver;
public:
    int id; // useful in log files
    double desiredPosition;
    double finalPosition;
    double weight; // how much the variable wants to 
                   // be at it's desired position
    double scale; // translates variable to another space
    double offset;
    Block *block;
    bool visited;
    bool fixedDesiredPosition;
    Constraints in;
    Constraints out;
    char *toString();
    inline Variable(const int id, const double desiredPos=-1.0, 
            const double weight=1.0, const double scale=1.0)
        : id(id)
        , desiredPosition(desiredPos)
        , weight(weight)
        , scale(scale)
        , offset(0)
        , block(NULL)
        , visited(false)
        , fixedDesiredPosition(false)
    {
    }
    double dfdv() const {
        return 2. * weight * ( position() - desiredPosition );
    }
private:
    double position() const {
        return (block->ps.scale*block->posn+offset)/scale;
    }
};


class Constraint
{
    friend std::ostream& operator <<(std::ostream &os,const Constraint &c);
public:
    Variable *left;
    Variable *right;
    double gap;
    double lm;
    Constraint(Variable *left, Variable *right, double gap, bool equality=false);
    ~Constraint();
    double slack() const;
    long timeStamp;
    bool active;
    const bool equality;
    bool unsatisfiable;
};
/*
 * A block structure defined over the variables such that each block contains
 * 1 or more variables, with the invariant that all constraints inside a block
 * are satisfied by keeping the variables fixed relative to one another
 */
class Blocks
{
public:
    Blocks(Variables const &vs);
    ~Blocks(void);
    void mergeLeft(Block *r);
    void mergeRight(Block *l);
    void split(Block *b, Block *&l, Block *&r, Constraint *c);
    std::list<Variable*> *totalOrder();
    void cleanup();
    double cost();
    
    size_t size() const;
    Block *at(size_t index) const;
    void insert(Block *block);

    long blockTimeCtr;
private:
    void dfsVisit(Variable *v, std::list<Variable*> *order);
    void removeBlock(Block *doomed);

    std::vector<Block *> m_blocks;
    Variables const &vs;
    int nvs;
};

inline size_t Blocks::size() const
{
    return m_blocks.size();
}

inline Block *Blocks::at(size_t index) const
{
    return m_blocks[index];
}

inline void Blocks::insert(Block *block)
{
    m_blocks.push_back(block);
}

struct UnsatisfiableException {
    Constraints path;
};
struct UnsatisfiedConstraint {
    UnsatisfiedConstraint(Constraint& c):c(c) {}
    Constraint& c;
};
/*
 * Variable Placement with Separation Constraints problem instance
 */
class IncSolver {
public:
    unsigned splitCnt;
    bool satisfy();
    bool solve();
    void moveBlocks();
    void splitBlocks();
    IncSolver(Variables const &vs, Constraints const &cs); 

    ~IncSolver();
    Variables const & getVariables() { return vs; }
protected:
    Blocks *bs;
    unsigned m;
    Constraints const &cs;
    unsigned n;
    Variables const &vs;
    void printBlocks();
    void copyResult();
private:
    bool constraintGraphIsCyclic(const unsigned n, Variable* const vs[]);
    bool blockGraphIsCyclic();
    Constraints inactive;
    Constraints violated;
    Constraint* mostViolated(Constraints &l);
};

struct delete_object
{
    template <typename T>
    void operator()(T *ptr){ delete ptr;}
};

extern Constraints constraintsRemovingRedundantEqualities(
        Variables const &vars, Constraints const &constraints);

}

#endif // ! USELIBVPSC

#endif // AVOID_VPSC_H

