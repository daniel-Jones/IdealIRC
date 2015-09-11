/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2014  Tom-Andre Barstad
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "../tscript.h"
#include "constants.h"

/*!
 * \param data Expression to solve
 * \param localVar Local variables
 * \param localBinVar Local binary variables
 *
 * This function solves an expression such as test == 1234
 * \return
 */
bool TScript::solveBool(QString &data, QHash<QString,QString> &localVar, QHash<QString,QByteArray> &localBinVar)
{
    /// This function solves the individual bool expressions like  test == 1234

    QString Q1;
    QString Q2;
    bool onQ2 = false; // Sets to true when we get an operator.

    // Enumerate all operators.
    // Add numbers every 5 for convinence, faster lookup when debugging.
    enum {
        NOOPERATOR = 0,
        NOT_EQ,
        TRIP_NOT_EQ,
        IS_EQ ,
        TRIP_IS_EQ,
        LESSTHAN = 5,
        GREATERTHAN,
        LESSTHANEQUAL,
        GREATERTHANEQUAL,
        ISON_CHANNEL,
        ISOPERATOR = 10,
        ISHALFOP,
        ISVOICED,
        ISREGULAR
    };

    short op = NOOPERATOR;

    for (int i = 0; i <= data.length()-1; i++) {
        QChar c = data[i];

        QString s_op = data.mid(i, 3);

        if (s_op == "===") {
            onQ2 = true;
            op = TRIP_IS_EQ;
            i += 3;
            continue;
        }

        if (s_op == "!==") {
            onQ2 = true;
            op = TRIP_NOT_EQ;
            i += 3;
            continue;
        }


        s_op = data.mid(i, 2);

        if (s_op == "==") {
            onQ2 = true;
            op = IS_EQ;
            i += 2;
            continue;
        }

        if (s_op == "!=") {
            onQ2 = true;
            op = NOT_EQ;
            i += 2;
            continue;
        }

        if (s_op == ">=") {
            onQ2 = true;
            op = GREATERTHANEQUAL;
            i += 2;
            continue;
        }

        if (s_op == "<=") {
            onQ2 = true;
            op = LESSTHANEQUAL;
            i += 2;
            continue;
        }

       // pointless for 1-char operator, s_op = data[i];

        if (c == '>') {
            onQ2 = true;
            op = GREATERTHAN;
            i += 1;
            continue;
        }

        if (c == '<') {
            onQ2 = true;
            op = LESSTHAN;
            i += 1;
            continue;
        }

        if (s_op == "?#") {
            onQ2 = true;
            op = ISON_CHANNEL;
            i += 2;
            continue;
        }

        if (s_op == "?@") {
            onQ2 = true;
            op = ISOPERATOR;
            i += 2;
            continue;
        }

        if (s_op == "?%") {
            onQ2 = true;
            op = ISHALFOP;
            i += 2;
            continue;
        }

        if (s_op == "?+") {
            onQ2 = true;
            op = ISVOICED;
            i += 2;
            continue;
        }

        if (s_op == "?-") {
            onQ2 = true;
            op = ISREGULAR;
            i += 2;
            continue;
        }

        if (onQ2)
            Q2 += c;
        else {
            if ((c == ' ') && ((data[i+1] == '!') || (data[i+1] == '=') || (data[i+1] == '<') || (data[i+1] == '>') || (data[i+1] == '?')))
                continue;
            Q1 += c;
        }
    }

    if (Q1 == "\\n")
        Q1 = "\n";
    if (Q2 == "\\n")
        Q2 = "\n";

    if (Q1.toUpper() == "$NULL")
        Q1.clear();
    if (Q2.toUpper() == "$NULL")
        Q2.clear();

    // TODO error checking missing.

    extract(Q1, localVar, localBinVar);
    extract(Q2, localVar, localBinVar);

    QString Q1u = Q1.toUpper();
    QString Q2u = Q2.toUpper();

    IConnection *c;
    if (op >= ISON_CHANNEL) // channel specific operations
        c = conList->value(*activeConn);

    switch (op) {
        case NOT_EQ:
            return (Q1u != Q2u);

        case TRIP_NOT_EQ:
            return (Q1 != Q2);

        case IS_EQ:
            return (Q1u == Q2u);

        case TRIP_IS_EQ:
            return (Q1 == Q2);

        case LESSTHAN:
            return (Q1.toDouble() < Q2.toDouble());

        case GREATERTHAN:
            return (Q1.toDouble() > Q2.toDouble());

        case LESSTHANEQUAL:
            return (Q1.toDouble() <= Q2.toDouble());

        case GREATERTHANEQUAL:
            return (Q1.toDouble() >= Q2.toDouble());

        case ISON_CHANNEL:
            return c->ial.sharesChannel(Q1, Q2);

        case ISOPERATOR:
            return c->ial.isOperator(Q1, Q2);

        case ISHALFOP:
            return c->ial.isHalfop(Q1, Q2);

        case ISVOICED:
            return c->ial.isVoiced(Q1, Q2);

        case ISREGULAR:
            return c->ial.isRegular(Q1, Q2);

        default:
            return false;
    }

    // Basically, we shouldn't reach here, but for safety, we make it return false.
    return false;
}

/*!
 * \param data Expression to solve
 * \param localVar Local variables
 * \param localBinVar Local binary variables
 *
 * \brief This function solves expressions such as: ((test == 1234) && (x < 5))
 *
 * First this function will solve the individual expressions (such as (test == 1234)) using solveBool() and return 1 or 0.\n
 * This function produces as such:\n
 * * 1. parameter enters as such:
 * data = ((%first == john) && (%last == doe))
 * 2. Converts every '%var == data' and alike using solveBool(), into:
 * data => exp = ((1) && (0))\n
 * This is then passed to the exprtk library for solving, and in this case, result will be 0.\n\n
 *
 * A more "complex" example:
 * data = (((%win == @draw1) || (%win == @draw2)) && (% > 100))
 * data parses via loop, with output in exp:
 * exp = (((1) + (0)) * (1))
 * ORing is same as plus, ANDing is same as multi.
 * solve string with math library, function results based upon that.
 * \return False if expression result is 0, true otherwise.
 */
bool TScript::solveLogic(QString &data, QHash<QString,QString> &localVar, QHash<QString,QByteArray> &localBinVar)
{
    int tl = data.length();

    if (tl == 0)
        return false;

    int nst = 0; // Paranthesis nesting level
    QString exp; // Expression to run with mathexp.h
    QString lt; // Logic testing, eg. "testvar != 1234"
    bool addLt = false;

    bool fnParanthesis = false;
    int fnpnest = 0;

    for (int i = 0; i <= tl-1; ++i) {
        QChar c = data[i]; // Current character
        QChar c1 = 0x00; // Next character

        if (i < tl-1) // Only get next character if we aren't on the end.
            c1 = data[i+1];

        if (c == '\\') { // BUG: extract() function shadows the escaping here.
            lt += c1;
            ++i;
            continue;
        }

        if (c == '$')
            fnParanthesis = true;

        if (fnParanthesis) {
            if ((fnpnest == 0) && ((c == ' ') || (c == ')'))) { // function was given no paranthesis, safe to continue.
                fnParanthesis = false;
                if (c == ')')
                    --i;
                else
                    lt += c;
                continue;
            }

            if (c == '(') { // Function paranthesis found, increment nest level of it...
                ++fnpnest;
                lt += c;
                continue;
            }

            if (c == ')') { // decrement nest level of function paranthesis...
                --fnpnest;
                lt += c;
                if (fnpnest == 0) // out of function paranthesises, next ones are likely to be the conditional testers.
                    fnParanthesis = false;
                continue;
            }
        }

        if (c == '(') {
            ++nst;
            exp += c;
            lt.clear();
            continue;
        }
        if ((c == ')') && (addLt == false)) {
            --nst;
            exp += c;
            if (nst == 0) {
                if (i < tl-1) {
                    // apparently we're on the end of parsing but looking on the numbers, something wrong probably happened.
                    // echo("Script error: Mismatching paranthesis count");
                    exp.clear();
                    break;
                }
                break; // Done, yay, etc
            }
            continue;
        }
        if ((c == '|') && (c1 == '|')) {
            // OR
            exp += '+';
            i++; // we're checking 2 characters, so increment one time now and it'll increment one by end of this loop.
            continue;
        }
        if ((c == '&') && (c1 == '&')) {
            // AND
            exp += '*';
            ++i; // we're checking 2 characters, so increment one time now and it'll increment one by end of this loop.
            continue;
        }

        // When we reach here, none of the above statements was true;
        // so we treat it as Logic Testing; add data to 'lt' until we reach paranthesis end - )

        addLt = true;

        if ((c == ')') && (addLt == true)) {
            int l = solveBool(lt, localVar, localBinVar);
            exp += QString::number(l) + ')';
            lt.clear();
            addLt = false;
            continue;
        }
        lt += c; // Add expression to lt
    }

    if (exp.length() > 0)
        if (ifn.calc(exp).toInt() > 0)
            return true;

    return false;
}
