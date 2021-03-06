/**
 * @file
 *
 * @brief
 *
 * @copyright BSD License (see doc/LICENSE.md or http://www.libelektra.org)
 */

#ifndef VISITOR_H
#define VISITOR_H

class TreeItem;
class TreeModel;

/**
 * @brief  The abstract Visitor class to support the visitor pattern.
 */
class Visitor
{

public:
	/**
	 * @brief The abstract method a visitor who wants to visit a ConfigNode needs to implement.
	 *
	 * @param node The visited ConfigNode
	 */
	virtual void visit (TreeItem & item) = 0;
};

#endif // VISITOR_H
