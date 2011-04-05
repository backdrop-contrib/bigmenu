<?php

/**
 * @file
 * Administrative page callbacks for bigmenu module.
 *
 * Large amounts copied from menu.admin.inc
 *
 * @author Dan (dman) Morrison dan@coders.co.nz
 * @version 2011
 */

/**
 * We re-use as much of the core menu rendering tools as possible.
 */
module_load_include('inc', 'menu', 'menu.admin');

/**
 * Form for editing an entire menu tree at once.
 *
 * Shows for one menu the menu items accessible to the current user and
 * relevant operations.
 *
 * Copied from core and modified to reduce recursion
 *
 * @see menu_overview_form
 */
function bigmenu_overview_form(&$form_state, $menu, $depth = 1) {
  global $menu_admin;
  $p_depth = 'p' . ($depth+3);
  $sql = "
    SELECT m.load_functions, m.to_arg_functions, m.access_callback, m.access_arguments, m.page_callback, m.page_arguments, m.title, m.title_callback, m.title_arguments, m.type, m.description, ml.*
    FROM {menu_links} ml LEFT JOIN {menu_router} m ON m.path = ml.router_path
    WHERE ml.menu_name = '%s'
    AND %s = 0
    ORDER BY p1 ASC, p2 ASC, p3 ASC, p4 ASC, p5 ASC, p6 ASC, p7 ASC, p8 ASC, p9 ASC";
  $result = db_query($sql, $menu['menu_name'], $p_depth);
  $tree = menu_tree_data($result);
  $node_links = array();
  menu_tree_collect_node_links($tree, $node_links);
  // We indicate that a menu administrator is running the menu access check.
  $menu_admin = TRUE;
  menu_tree_check_access($tree, $node_links);
  $menu_admin = FALSE;

  $form = _bigmenu_overview_tree_form($tree, $depth);
  $form['#menu'] =  $menu;
  if (element_children($form)) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save configuration'),
    );
  }
  else {
    $form['empty_menu'] = array('#value' => t('There are no menu items yet.'));
  }
  return $form;
}


/**
 * Form for editing the immediate children of the given menu item id
 *
 * Invoked by menu callback or by json request
 *
 * @return FAPI form.
 */
function bigmenu_slice_form(&$form_state, $menu, $menu_link, $depth = 2) {
  $menu_slice = menu_get_menus();

  // DB lookup the children of this item
  global $menu_admin;
  $p_depth = 'p' . $menu_link['depth'];
  $sql = "
    SELECT m.load_functions, m.to_arg_functions, m.access_callback, m.access_arguments, m.page_callback, m.page_arguments, m.title, m.title_callback, m.title_arguments, m.type, m.description, ml.*
    FROM {menu_links} ml LEFT JOIN {menu_router} m ON m.path = ml.router_path
    WHERE ml.menu_name = '%s'
    AND %s = %d
    ORDER BY p1 ASC, p2 ASC, p3 ASC, p4 ASC, p5 ASC, p6 ASC, p7 ASC, p8 ASC, p9 ASC";
  $result = db_query($sql, $menu['menu_name'], $p_depth, $menu_link['mlid']);
  $tree = menu_tree_data($result);

  $node_links = array();
  menu_tree_collect_node_links($tree, $node_links);
  // We indicate that a menu administrator is running the menu access check.
  $menu_admin = TRUE;
  menu_tree_check_access($tree, $node_links);
  $menu_admin = FALSE;

  // When doing a slice, don't show the actual parent link item, just the children
  foreach($tree as $ix => $data) {
    unset($tree[$ix]['link']);
  }
  // (there was only one, but I didn't know its name)

  $form = _bigmenu_overview_tree_form($tree, $depth);

  $form['#theme'] = 'bigmenu_overview_form';
  $form['#menu'] =  $menu;
  if (element_children($form)) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save configuration'),
    );
  }
  else {
    $form['empty_menu'] = array('#value' => t('There are no menu items yet.'));
  }
  return $form;
}

/**
 * Return a submenu slice form in json format
 * Menu callback, invoked by AJAX
 *
 * Drupals form cache would not allow me to insert arbitrary fields into a form
 * without letting the server know about it.
 */
function bigmenu_slice_form_js($menu, $menu_link, $form_id, $form_build_id) {
  $depth = 2;

  $form_state = array();
  $slice_form_id = 'bigmenu_slice_form';

  $output = drupal_get_form('bigmenu_slice_form', $menu, $menu_link, $depth);
  # Or maybe,
  # Need to do the things that drupal_get_form would do,
  # but retain the form_object for the next step also.
  # Maybe not?
  #  $slice_form = drupal_retrieve_form($slice_form_id, $form_state, $menu, $menu_link, $depth);
  #  drupal_prepare_form($slice_form_id, $slice_form, $form_state);
  #  drupal_process_form($slice_form_id, $slice_form, $form_state);
  #  $output = drupal_render_form($slice_form_id, $slice_form);

  // This returns data to the browser immediately.
  drupal_json(array('status' => TRUE, 'data' => $output));

  // After that we will do the slower job of updating the form cache.
  // Reload the bigmenu_overview_form, with the same args (the menu name) as before,

  // What are the following three lines doing? Search me.
  // I read the docs on drupal_rebuild_form() and took a guess.
  $form_state = array();
  $args = array($form_state, $menu);
  $form = drupal_rebuild_form($form_id, $form_state, $args, $form_build_id);

  // Just calling drupal_rebuild_form will also re-cache it with the new fields?
  // How does it know?

  # I thought I would have to:
  # Then insert the new fields into the cached overview form from the slice form.

  # $form += $slice_form;

  exit;
}


/**
 * Recursive helper function for menu_overview_form().
 *
 * @see _menu_overview_tree_form()
 *
 * Copied from core, and modified to limit recursion
 */
function _bigmenu_overview_tree_form($tree, $depth = 0) {
  static $form = array('#tree' => TRUE);

  foreach ($tree as $data) {
    $title = '';
    $item = $data['link'];
    // Don't show callbacks; these have $item['hidden'] < 0.
    if ($item && $item['hidden'] >= 0) {
      $mlid = 'mlid:'. $item['mlid'];
      $form[$mlid]['#item'] = $item;
      $form[$mlid]['#attributes'] = $item['hidden'] ? array('class' => 'menu-disabled') : array('class' => 'menu-enabled');
      $form[$mlid]['title']['#value'] = l($item['title'], $item['href'], $item['localized_options']) . ($item['hidden'] ? ' ('. t('disabled') .')' : '');
      $form[$mlid]['hidden'] = array(
        '#type' => 'checkbox',
        '#default_value' => !$item['hidden'],
      );
      $form[$mlid]['expanded'] = array(
        '#type' => 'checkbox',
        '#default_value' => $item['expanded'],
      );
      $form[$mlid]['weight'] = array(
        '#type' => 'weight',
        '#delta' => 50,
        '#default_value' => isset($form_state[$mlid]['weight']) ? $form_state[$mlid]['weight'] : $item['weight'],
      );
      $form[$mlid]['mlid'] = array(
        '#type' => 'hidden',
        #'#type' => 'textfield', '#size' => 5,
        '#value' => $item['mlid'],
      );
      $form[$mlid]['plid'] = array(
        '#type' => 'textfield',
        '#default_value' => isset($form_state[$mlid]['plid']) ? $form_state[$mlid]['plid'] : $item['plid'],
        '#size' => 6,
      );
      // Build a list of operations.
      $operations = array();
      $operations['edit'] = l(t('edit'), 'admin/build/menu/item/'. $item['mlid'] .'/edit');
      // Only items created by the menu module can be deleted.
      if ($item['module'] == 'menu' || $item['updated'] == 1) {
        $operations['delete'] = l(t('delete'), 'admin/build/menu/item/'. $item['mlid'] .'/delete');
      }
      // Set the reset column.
      elseif ($item['module'] == 'system' && $item['customized']) {
        $operations['reset'] = l(t('reset'), 'admin/build/menu/item/'. $item['mlid'] .'/reset');
      }

      $form[$mlid]['operations'] = array();
      foreach ($operations as $op => $value) {
        $form[$mlid]['operations'][$op] = array('#value' => $value);
      }
    }

    if ($data['below']) {
      if ($depth > 0) {
        _bigmenu_overview_tree_form($data['below'], $depth - 1);
      }
      else {
        // 'below' contains both immediate children and something else
        $strings = array(
          '!show_children' => t('Show children'),
          '%count' => count($data['below']),
          '!tooltip' => t('Click to expand and show child items'),
        );
        $text = strtr('<span class="bigmenu-toggle">+</span> <span class="count" title="!tooltip"><span class="hide-show">!show_children</span> (%count)</span>', $strings);
        $indicator = l($text, "admin/build/bigmenu-customize/{$item['menu_name']}/subform/{$item['mlid']}", array('html' => TRUE, attributes => array('class' => 'bigmenu-childindictor')));
        $form[$mlid]['title']['#value'] .= '<br/>' . $indicator;
        $form[$mlid]['#attributes']['class'] = 'bigmenu-collapsed';

      }
    }
  }
  return $form;
}

/**
 * Autocomplete callback to find page aliases
 *
 * Returns a json array with suggestions based on an entered string
 *
 * Matches from the beginning only
 * Removes http etc if you left it in.
 * Only aliases that also match a menu link item path are returned.
 * The key used is the mlid, not the alias id or the path.
 */
function bigmenu_alias_autocomplete() {
  // The URL callback may have any number of slashes in it, which will have been split out.
  $args = func_get_args();
  $string = join('/', $args);
  // if it was an URL, one of the slashes got squashed - fixit
  $string = preg_replace('|http:/(\w)|', 'http://$1', $string);

  if (valid_url($string)) {
    $url_parts = parse_url($string);
    $string = ltrim($url_parts['path'], '/');;
  }

  $sql = "
    SELECT u.src, u.dst, ml.mlid FROM {url_alias} u
    JOIN menu_links ml ON ml.link_path = u.src
    WHERE LOWER(u.dst) LIKE LOWER('%s%%')
  ";
  $result = db_query_range($sql, $string, 0, 10);
  $matches = array();
  while ($alias = db_fetch_object($result)) {
    $id = check_plain($alias->dst) . " [mlid:{$alias->mlid}]";
    $matches[$id] = check_plain($alias->dst);
  }
  print drupal_to_js($matches);
  exit();
}


/**
 * Theme the menu overview form into a table.
 *
 * A stub to core. No need to change anything. It just adds an extra js file and
 * passes it through.
 *
 * @ingroup themeable
 */
function theme_bigmenu_overview_form($form) {
  drupal_add_js(drupal_get_path('module', 'bigmenu') . '/bigmenu.js');
  drupal_add_css(drupal_get_path('module', 'bigmenu') . '/bigmenu.css');
  return theme('menu_overview_form', $form);
}

/**
 * A stub to core
 * ? should just use #submit?
 */
function bigmenu_overview_form_submit($form, &$form_state) {
  return menu_overview_form_submit($form, $form_state);
}
