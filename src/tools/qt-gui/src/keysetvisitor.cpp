/**
 * @file
 *
 * @brief
 *
 * @copyright BSD License (see doc/LICENSE.md or http://www.libelektra.org)
 */

#include "keysetvisitor.hpp"
#include "treemodel.hpp"

using namespace kdb;

KeySetVisitor::KeySetVisitor ()
{
}

void KeySetVisitor::visit (TreeItem & item)
{
//	Key key = item.key();

//	if (key && key.isValid ())
//	{
//		m_set.append (key);
//	}
}

void KeySetVisitor::visit (TreeModel * model)
{
//	foreach (TreeItemPtr node, model->model ())
//	{
//		node->accept (*this);
//	}
}

KeySet KeySetVisitor::getKeySet ()
{
//	return m_set.dup ();
}
