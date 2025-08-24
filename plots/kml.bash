awk 'BEGIN{
  print "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  print "<kml xmlns=\"http://www.opengis.net/kml/2.2\"><Document>";
  print "<name>Receiver track</name><Placemark><name>Receiver track</name>";
  print "<Style><LineStyle><color>ff0066ff</color><width>3</width></LineStyle></Style>";
  print "<LineString><tessellate>1</tessellate><coordinates>";
}
{ printf "  %.8f,%.8f,0\n", $2, $1 }   # lon,lat,alt
END{
  print "</coordinates></LineString></Placemark></Document></kml>";
}' plots/receiver_track_geo.dat > plots/receiver_track.kml
