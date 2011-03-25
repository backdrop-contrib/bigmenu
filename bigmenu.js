/**
 * @file add AJAX loading to the manu admin screen
 *
 * @author dman dan@coders.co.nz
 */
 
Drupal.behaviors.bigmenu = function(context) {
  // add click actions to all the child indicators
  $('form#bigmenu-overview-form .bigmenu-childindictor')
    .addClass('clickable')
    .click(
      function(event){
        // Don't let the normal href_ click happen
        event.stopPropagation();

        // jquery 1.3+
        // var parentRow = $(this).closest('tr');
        var parentRow = $(this).parents('tr').get();

        // Prevent double-clicks
        if ($(parentRow).hasClass("bigmenu-processing")) {
          return;
        }
        $(parentRow).addClass("bigmenu-processing")
        

        // set thobber
        $('.bigmenu-expand', $(this))
          .addClass("ahah-progress")
          .html('<div class="throbber">&nbsp;</div>');

          // Find the mlid of the cell we are in ...or not. 
          // This clicked item has the href call we need to make built in
          // just add 'js' to the end
          var url = $(this).attr('href') + '/js';

          // Make an ajax call for the child items.

          $.ajax({
            dataType: 'json',
            url: url,
            success: function(data, textStatus, XMLHttpRequest) {
              // remove thobber        
              $('.bigmenu-childindictor', $(parentRow)).remove();
              $(parentRow).removeClass('bigmenu-processing')
                .addClass('bigmenu-processed')

              if (data.status) {
                // Shift the rows into this form.
                
                var new_form = $(data.data);

                // add each tr in the retrieved form  after the parent row
                $('tr', new_form).each(function(index) {
                    if ($('th', this).length == 0) {
                      $(parentRow).after(this);
                      // TODO - an animation of some sort
                      // Don't want to reverse the order, so adjust the parentrow pointer
                      parentRow = this
                    }
                });
                // Attach any required behaviors to the table
                // thus, the further child expanders, and the tabledrag again.
                // tabledrag doesn't like running twice, so we have to remove some evidence
                $('#menu-overview').removeClass('tabledrag-processed');
                $('#menu-overview .tabledrag-handle').remove();
                
                Drupal.attachBehaviors();
              }
              else {
                // Failure...
                alert(Drupal.t('AJAX error fetching submenu.'));
                $('.bigmenu-childindictor', $(parentRow)).remove();
              }
            },
            error: function(XMLHttpRequest, textStatus, errorThrown) {
              // Failure...
              $('.bigmenu-childindictor', $(parentRow)).remove();
              alert(Drupal.t('Error fetching submenu: @error', { '@error': textStatus }));
            }
          });
        return false;
      }
    );
}
