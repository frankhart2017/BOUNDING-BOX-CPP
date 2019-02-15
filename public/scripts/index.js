var x1_send, y1_send, x2_send, y2_send;

$('#img').imgAreaSelect({
    handles: true,
    onSelectEnd: function(img, selection) {
      x1_send = selection.x1,
      y1_send = selection.y1,
      x2_send = selection.x2,
      y2_send = selection.y2
    }
});

$("#next").click(function() {

    var otype = $("#otype").val();

    if(otype === '') {
      alert("Enter object type");
    } else if(x1_send === '') {
      alert("Select bounding box");
    } else {
      $.ajax({
        url: "/image",
        type: "post",
        dataType: "json",
        contentType: 'application/json',
        data: JSON.stringify({"x1": x1_send, "x2": x2_send, "y1": y1_send, "y2": y2_send, "otype": otype}),
        success: function(result) {
          window.location.reload();
        }
      });

      location.href = location.href;
    }

});
