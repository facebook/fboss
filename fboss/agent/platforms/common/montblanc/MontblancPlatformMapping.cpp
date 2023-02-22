/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/montblanc/MontblancPlatformMapping.h"

namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
  "ports": {
    "1": {
        "mapping": {
          "id": 1,
          "name": "eth1/14/1",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC0",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC0",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC0",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                2
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "2": {
        "mapping": {
          "id": 2,
          "name": "eth1/14/2",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC0",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC0",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC0",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "3": {
        "mapping": {
          "id": 3,
          "name": "eth1/1/1",
          "controllingPort": 3,
          "pins": [
            {
              "a": {
                "chip": "BC1",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC1",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC1",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC1",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                4
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "4": {
        "mapping": {
          "id": 4,
          "name": "eth1/1/2",
          "controllingPort": 3,
          "pins": [
            {
              "a": {
                "chip": "BC1",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC1",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC1",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC1",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/1",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC1",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/1",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "11": {
        "mapping": {
          "id": 11,
          "name": "eth1/18/1",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC2",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC2",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC2",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                12
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "12": {
        "mapping": {
          "id": 12,
          "name": "eth1/18/2",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC2",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC2",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC2",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "13": {
        "mapping": {
          "id": 13,
          "name": "eth1/29/1",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "BC3",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC3",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC3",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC3",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                14
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "14": {
        "mapping": {
          "id": 14,
          "name": "eth1/29/2",
          "controllingPort": 13,
          "pins": [
            {
              "a": {
                "chip": "BC3",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC3",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC3",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC3",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/29",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC3",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/29",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "22": {
        "mapping": {
          "id": 22,
          "name": "eth1/2/1",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC4",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC4",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC4",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                23
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "23": {
        "mapping": {
          "id": 23,
          "name": "eth1/2/2",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC4",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC4",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC4",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "24": {
        "mapping": {
          "id": 24,
          "name": "eth1/13/1",
          "controllingPort": 24,
          "pins": [
            {
              "a": {
                "chip": "BC5",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC5",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC5",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC5",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                25
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "25": {
        "mapping": {
          "id": 25,
          "name": "eth1/13/2",
          "controllingPort": 24,
          "pins": [
            {
              "a": {
                "chip": "BC5",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC5",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC5",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC5",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/13",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC5",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/13",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "33": {
        "mapping": {
          "id": 33,
          "name": "eth1/17/1",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC6",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC6",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC6",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                34
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "34": {
        "mapping": {
          "id": 34,
          "name": "eth1/17/2",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC6",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC6",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC6",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/17",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/17",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "35": {
        "mapping": {
          "id": 35,
          "name": "eth1/30/1",
          "controllingPort": 35,
          "pins": [
            {
              "a": {
                "chip": "BC7",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC7",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC7",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC7",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                36
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "36": {
        "mapping": {
          "id": 36,
          "name": "eth1/30/2",
          "controllingPort": 35,
          "pins": [
            {
              "a": {
                "chip": "BC7",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC7",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC7",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC7",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC7",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "44": {
        "mapping": {
          "id": 44,
          "name": "eth1/3/1",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC8",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC8",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC8",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                45
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "45": {
        "mapping": {
          "id": 45,
          "name": "eth1/3/2",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC8",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC8",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC8",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "46": {
        "mapping": {
          "id": 46,
          "name": "eth1/16/1",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "BC9",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC9",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC9",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC9",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                47
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "47": {
        "mapping": {
          "id": 47,
          "name": "eth1/16/2",
          "controllingPort": 46,
          "pins": [
            {
              "a": {
                "chip": "BC9",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC9",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC9",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC9",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/16",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC9",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/16",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "55": {
        "mapping": {
          "id": 55,
          "name": "eth1/20/1",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC10",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC10",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC10",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                56
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "56": {
        "mapping": {
          "id": 56,
          "name": "eth1/20/2",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC10",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC10",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC10",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/20",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/20",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "57": {
        "mapping": {
          "id": 57,
          "name": "eth1/31/1",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "BC11",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC11",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC11",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC11",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                58
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "58": {
        "mapping": {
          "id": 58,
          "name": "eth1/31/2",
          "controllingPort": 57,
          "pins": [
            {
              "a": {
                "chip": "BC11",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC11",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC11",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC11",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC11",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "66": {
        "mapping": {
          "id": 66,
          "name": "eth1/4/1",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC12",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC12",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC12",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                67
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "67": {
        "mapping": {
          "id": 67,
          "name": "eth1/4/2",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC12",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC12",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC12",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/4",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/4",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "68": {
        "mapping": {
          "id": 68,
          "name": "eth1/15/1",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "BC13",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC13",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC13",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC13",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                69
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "69": {
        "mapping": {
          "id": 69,
          "name": "eth1/15/2",
          "controllingPort": 68,
          "pins": [
            {
              "a": {
                "chip": "BC13",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC13",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC13",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC13",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC13",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "77": {
        "mapping": {
          "id": 77,
          "name": "eth1/19/1",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC14",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC14",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC14",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                78
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "78": {
        "mapping": {
          "id": 78,
          "name": "eth1/19/2",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC14",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC14",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC14",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "79": {
        "mapping": {
          "id": 79,
          "name": "eth1/32/1",
          "controllingPort": 79,
          "pins": [
            {
              "a": {
                "chip": "BC15",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC15",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC15",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC15",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                80
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "80": {
        "mapping": {
          "id": 80,
          "name": "eth1/32/2",
          "controllingPort": 79,
          "pins": [
            {
              "a": {
                "chip": "BC15",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC15",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC15",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC15",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/32",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC15",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/32",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "88": {
        "mapping": {
          "id": 88,
          "name": "eth1/5/1",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC16",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC16",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC16",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                89
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "89": {
        "mapping": {
          "id": 89,
          "name": "eth1/5/2",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC16",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC16",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC16",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/5",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/5",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "90": {
        "mapping": {
          "id": 90,
          "name": "eth1/10/1",
          "controllingPort": 90,
          "pins": [
            {
              "a": {
                "chip": "BC17",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC17",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC17",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC17",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                91
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "91": {
        "mapping": {
          "id": 91,
          "name": "eth1/10/2",
          "controllingPort": 90,
          "pins": [
            {
              "a": {
                "chip": "BC17",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC17",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC17",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC17",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC17",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "99": {
        "mapping": {
          "id": 99,
          "name": "eth1/22/1",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC18",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC18",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC18",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                100
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "100": {
        "mapping": {
          "id": 100,
          "name": "eth1/22/2",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC18",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC18",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC18",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "101": {
        "mapping": {
          "id": 101,
          "name": "eth1/25/1",
          "controllingPort": 101,
          "pins": [
            {
              "a": {
                "chip": "BC19",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC19",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC19",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC19",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                102
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "102": {
        "mapping": {
          "id": 102,
          "name": "eth1/25/2",
          "controllingPort": 101,
          "pins": [
            {
              "a": {
                "chip": "BC19",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC19",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC19",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC19",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/25",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC19",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/25",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "110": {
        "mapping": {
          "id": 110,
          "name": "eth1/6/1",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC20",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC20",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC20",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                111
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "111": {
        "mapping": {
          "id": 111,
          "name": "eth1/6/2",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC20",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC20",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC20",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "112": {
        "mapping": {
          "id": 112,
          "name": "eth1/9/1",
          "controllingPort": 112,
          "pins": [
            {
              "a": {
                "chip": "BC21",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC21",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC21",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC21",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                113
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "113": {
        "mapping": {
          "id": 113,
          "name": "eth1/9/2",
          "controllingPort": 112,
          "pins": [
            {
              "a": {
                "chip": "BC21",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC21",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC21",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC21",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/9",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC21",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/9",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "121": {
        "mapping": {
          "id": 121,
          "name": "eth1/21/1",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC22",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC22",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC22",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                122
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "122": {
        "mapping": {
          "id": 122,
          "name": "eth1/21/2",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC22",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC22",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC22",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/21",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/21",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "123": {
        "mapping": {
          "id": 123,
          "name": "eth1/26/1",
          "controllingPort": 123,
          "pins": [
            {
              "a": {
                "chip": "BC23",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC23",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC23",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC23",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                124
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "124": {
        "mapping": {
          "id": 124,
          "name": "eth1/26/2",
          "controllingPort": 123,
          "pins": [
            {
              "a": {
                "chip": "BC23",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC23",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC23",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC23",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC23",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "132": {
        "mapping": {
          "id": 132,
          "name": "eth1/7/1",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC24",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC24",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC24",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                133
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "133": {
        "mapping": {
          "id": 133,
          "name": "eth1/7/2",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC24",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC24",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC24",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "134": {
        "mapping": {
          "id": 134,
          "name": "eth1/12/1",
          "controllingPort": 134,
          "pins": [
            {
              "a": {
                "chip": "BC25",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC25",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC25",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC25",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                135
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "135": {
        "mapping": {
          "id": 135,
          "name": "eth1/12/2",
          "controllingPort": 134,
          "pins": [
            {
              "a": {
                "chip": "BC25",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC25",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC25",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC25",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/12",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC25",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/12",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "143": {
        "mapping": {
          "id": 143,
          "name": "eth1/24/1",
          "controllingPort": 143,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC26",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC26",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC26",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                144
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "144": {
        "mapping": {
          "id": 144,
          "name": "eth1/24/2",
          "controllingPort": 143,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC26",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC26",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC26",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/24",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/24",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "145": {
        "mapping": {
          "id": 145,
          "name": "eth1/27/1",
          "controllingPort": 145,
          "pins": [
            {
              "a": {
                "chip": "BC27",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC27",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC27",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC27",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                146
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "146": {
        "mapping": {
          "id": 146,
          "name": "eth1/27/2",
          "controllingPort": 145,
          "pins": [
            {
              "a": {
                "chip": "BC27",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC27",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC27",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC27",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC27",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "154": {
        "mapping": {
          "id": 154,
          "name": "eth1/8/1",
          "controllingPort": 154,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC28",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC28",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC28",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                155
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "155": {
        "mapping": {
          "id": 155,
          "name": "eth1/8/2",
          "controllingPort": 154,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC28",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC28",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC28",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/8",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/8",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "156": {
        "mapping": {
          "id": 156,
          "name": "eth1/11/1",
          "controllingPort": 156,
          "pins": [
            {
              "a": {
                "chip": "BC29",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC29",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC29",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC29",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                157
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "157": {
        "mapping": {
          "id": 157,
          "name": "eth1/11/2",
          "controllingPort": 156,
          "pins": [
            {
              "a": {
                "chip": "BC29",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC29",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC29",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC29",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC29",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "165": {
        "mapping": {
          "id": 165,
          "name": "eth1/28/1",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC30",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC30",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC30",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                166
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "166": {
        "mapping": {
          "id": 166,
          "name": "eth1/28/2",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC30",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC30",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC30",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/28",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/28",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "167": {
        "mapping": {
          "id": 167,
          "name": "eth1/23/1",
          "controllingPort": 167,
          "pins": [
            {
              "a": {
                "chip": "BC31",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC31",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC31",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC31",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                168
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "168": {
        "mapping": {
          "id": 168,
          "name": "eth1/23/2",
          "controllingPort": 167,
          "pins": [
            {
              "a": {
                "chip": "BC31",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC31",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC31",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC31",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC31",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "176": {
        "mapping": {
          "id": 176,
          "name": "eth1/47/1",
          "controllingPort": 176,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC32",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC32",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC32",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                177
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "177": {
        "mapping": {
          "id": 177,
          "name": "eth1/47/2",
          "controllingPort": 176,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC32",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC32",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC32",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "178": {
        "mapping": {
          "id": 178,
          "name": "eth1/36/1",
          "controllingPort": 178,
          "pins": [
            {
              "a": {
                "chip": "BC33",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/36",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC33",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/36",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC33",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/36",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC33",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/36",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                179
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "179": {
        "mapping": {
          "id": 179,
          "name": "eth1/36/2",
          "controllingPort": 178,
          "pins": [
            {
              "a": {
                "chip": "BC33",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/36",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC33",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/36",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC33",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/36",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC33",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/36",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC33",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/36",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "187": {
        "mapping": {
          "id": 187,
          "name": "eth1/51/1",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC34",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC34",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC34",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                188
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "188": {
        "mapping": {
          "id": 188,
          "name": "eth1/51/2",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC34",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC34",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC34",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "189": {
        "mapping": {
          "id": 189,
          "name": "eth1/64/1",
          "controllingPort": 189,
          "pins": [
            {
              "a": {
                "chip": "BC35",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/64",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC35",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/64",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC35",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/64",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC35",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/64",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                190
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "190": {
        "mapping": {
          "id": 190,
          "name": "eth1/64/2",
          "controllingPort": 189,
          "pins": [
            {
              "a": {
                "chip": "BC35",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/64",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC35",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/64",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC35",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/64",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC35",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/64",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC35",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/64",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "198": {
        "mapping": {
          "id": 198,
          "name": "eth1/35/1",
          "controllingPort": 198,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC36",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC36",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC36",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                199
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "199": {
        "mapping": {
          "id": 199,
          "name": "eth1/35/2",
          "controllingPort": 198,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC36",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC36",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC36",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "200": {
        "mapping": {
          "id": 200,
          "name": "eth1/48/1",
          "controllingPort": 200,
          "pins": [
            {
              "a": {
                "chip": "BC37",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/48",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC37",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/48",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC37",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/48",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC37",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/48",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                201
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "201": {
        "mapping": {
          "id": 201,
          "name": "eth1/48/2",
          "controllingPort": 200,
          "pins": [
            {
              "a": {
                "chip": "BC37",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/48",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC37",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/48",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC37",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/48",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC37",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/48",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC37",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/48",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "209": {
        "mapping": {
          "id": 209,
          "name": "eth1/52/1",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/52",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC38",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/52",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC38",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/52",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC38",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/52",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                210
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "210": {
        "mapping": {
          "id": 210,
          "name": "eth1/52/2",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/52",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC38",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/52",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC38",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/52",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC38",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/52",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/52",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "211": {
        "mapping": {
          "id": 211,
          "name": "eth1/63/1",
          "controllingPort": 211,
          "pins": [
            {
              "a": {
                "chip": "BC39",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC39",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC39",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC39",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                212
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "212": {
        "mapping": {
          "id": 212,
          "name": "eth1/63/2",
          "controllingPort": 211,
          "pins": [
            {
              "a": {
                "chip": "BC39",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC39",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC39",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC39",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC39",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "220": {
        "mapping": {
          "id": 220,
          "name": "eth1/34/1",
          "controllingPort": 220,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC40",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC40",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC40",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                221
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "221": {
        "mapping": {
          "id": 221,
          "name": "eth1/34/2",
          "controllingPort": 220,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC40",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC40",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC40",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "222": {
        "mapping": {
          "id": 222,
          "name": "eth1/45/1",
          "controllingPort": 222,
          "pins": [
            {
              "a": {
                "chip": "BC41",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/45",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC41",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/45",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC41",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/45",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC41",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/45",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                223
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "223": {
        "mapping": {
          "id": 223,
          "name": "eth1/45/2",
          "controllingPort": 222,
          "pins": [
            {
              "a": {
                "chip": "BC41",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/45",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC41",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/45",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC41",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/45",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC41",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/45",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC41",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/45",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "231": {
        "mapping": {
          "id": 231,
          "name": "eth1/49/1",
          "controllingPort": 231,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/49",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC42",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/49",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC42",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/49",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC42",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/49",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                232
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "232": {
        "mapping": {
          "id": 232,
          "name": "eth1/49/2",
          "controllingPort": 231,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/49",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC42",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/49",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC42",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/49",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC42",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/49",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/49",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "233": {
        "mapping": {
          "id": 233,
          "name": "eth1/62/1",
          "controllingPort": 233,
          "pins": [
            {
              "a": {
                "chip": "BC43",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC43",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC43",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC43",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                234
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "234": {
        "mapping": {
          "id": 234,
          "name": "eth1/62/2",
          "controllingPort": 233,
          "pins": [
            {
              "a": {
                "chip": "BC43",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC43",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC43",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC43",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC43",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "242": {
        "mapping": {
          "id": 242,
          "name": "eth1/33/1",
          "controllingPort": 242,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/33",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC44",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/33",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC44",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/33",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC44",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/33",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                243
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "243": {
        "mapping": {
          "id": 243,
          "name": "eth1/33/2",
          "controllingPort": 242,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/33",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC44",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/33",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC44",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/33",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC44",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/33",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/33",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "244": {
        "mapping": {
          "id": 244,
          "name": "eth1/46/1",
          "controllingPort": 244,
          "pins": [
            {
              "a": {
                "chip": "BC45",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC45",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC45",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC45",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                245
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "245": {
        "mapping": {
          "id": 245,
          "name": "eth1/46/2",
          "controllingPort": 244,
          "pins": [
            {
              "a": {
                "chip": "BC45",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC45",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC45",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC45",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC45",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "253": {
        "mapping": {
          "id": 253,
          "name": "eth1/50/1",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC46",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC46",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC46",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                254
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "254": {
        "mapping": {
          "id": 254,
          "name": "eth1/50/2",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC46",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC46",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC46",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "255": {
        "mapping": {
          "id": 255,
          "name": "eth1/61/1",
          "controllingPort": 255,
          "pins": [
            {
              "a": {
                "chip": "BC47",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/61",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC47",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/61",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC47",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/61",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC47",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/61",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                256
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "256": {
        "mapping": {
          "id": 256,
          "name": "eth1/61/2",
          "controllingPort": 255,
          "pins": [
            {
              "a": {
                "chip": "BC47",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/61",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC47",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/61",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC47",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/61",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC47",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/61",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC47",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/61",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "264": {
        "mapping": {
          "id": 264,
          "name": "eth1/40/1",
          "controllingPort": 264,
          "pins": [
            {
              "a": {
                "chip": "BC48",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/40",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC48",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/40",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC48",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/40",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC48",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/40",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                265
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "265": {
        "mapping": {
          "id": 265,
          "name": "eth1/40/2",
          "controllingPort": 264,
          "pins": [
            {
              "a": {
                "chip": "BC48",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/40",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC48",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/40",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC48",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/40",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC48",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/40",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/40",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "266": {
        "mapping": {
          "id": 266,
          "name": "eth1/43/1",
          "controllingPort": 266,
          "pins": [
            {
              "a": {
                "chip": "BC49",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC49",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC49",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC49",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                267
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "267": {
        "mapping": {
          "id": 267,
          "name": "eth1/43/2",
          "controllingPort": 266,
          "pins": [
            {
              "a": {
                "chip": "BC49",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC49",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC49",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC49",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC49",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "275": {
        "mapping": {
          "id": 275,
          "name": "eth1/55/1",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "BC50",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC50",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC50",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC50",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                276
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "276": {
        "mapping": {
          "id": 276,
          "name": "eth1/55/2",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "BC50",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC50",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC50",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC50",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "277": {
        "mapping": {
          "id": 277,
          "name": "eth1/60/1",
          "controllingPort": 277,
          "pins": [
            {
              "a": {
                "chip": "BC51",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/60",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC51",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/60",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC51",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/60",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC51",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/60",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                278
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "278": {
        "mapping": {
          "id": 278,
          "name": "eth1/60/2",
          "controllingPort": 277,
          "pins": [
            {
              "a": {
                "chip": "BC51",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/60",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC51",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/60",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC51",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/60",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC51",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/60",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC51",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/60",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "286": {
        "mapping": {
          "id": 286,
          "name": "eth1/39/1",
          "controllingPort": 286,
          "pins": [
            {
              "a": {
                "chip": "BC52",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC52",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC52",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC52",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                287
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "287": {
        "mapping": {
          "id": 287,
          "name": "eth1/39/2",
          "controllingPort": 286,
          "pins": [
            {
              "a": {
                "chip": "BC52",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC52",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC52",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC52",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "288": {
        "mapping": {
          "id": 288,
          "name": "eth1/44/1",
          "controllingPort": 288,
          "pins": [
            {
              "a": {
                "chip": "BC53",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/44",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC53",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/44",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC53",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/44",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC53",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/44",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                289
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "289": {
        "mapping": {
          "id": 289,
          "name": "eth1/44/2",
          "controllingPort": 288,
          "pins": [
            {
              "a": {
                "chip": "BC53",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/44",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC53",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/44",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC53",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/44",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC53",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/44",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC53",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/44",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "297": {
        "mapping": {
          "id": 297,
          "name": "eth1/56/1",
          "controllingPort": 297,
          "pins": [
            {
              "a": {
                "chip": "BC54",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/56",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC54",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/56",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC54",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/56",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC54",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/56",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                298
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "298": {
        "mapping": {
          "id": 298,
          "name": "eth1/56/2",
          "controllingPort": 297,
          "pins": [
            {
              "a": {
                "chip": "BC54",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/56",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC54",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/56",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC54",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/56",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC54",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/56",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/56",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "299": {
        "mapping": {
          "id": 299,
          "name": "eth1/59/1",
          "controllingPort": 299,
          "pins": [
            {
              "a": {
                "chip": "BC55",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC55",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC55",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC55",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                300
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "300": {
        "mapping": {
          "id": 300,
          "name": "eth1/59/2",
          "controllingPort": 299,
          "pins": [
            {
              "a": {
                "chip": "BC55",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC55",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC55",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC55",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC55",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "308": {
        "mapping": {
          "id": 308,
          "name": "eth1/38/1",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "BC56",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC56",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC56",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC56",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                309
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "309": {
        "mapping": {
          "id": 309,
          "name": "eth1/38/2",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "BC56",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC56",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC56",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC56",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "310": {
        "mapping": {
          "id": 310,
          "name": "eth1/41/1",
          "controllingPort": 310,
          "pins": [
            {
              "a": {
                "chip": "BC57",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/41",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC57",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/41",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC57",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/41",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC57",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/41",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                311
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "311": {
        "mapping": {
          "id": 311,
          "name": "eth1/41/2",
          "controllingPort": 310,
          "pins": [
            {
              "a": {
                "chip": "BC57",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/41",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC57",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/41",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC57",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/41",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC57",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/41",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC57",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/41",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "319": {
        "mapping": {
          "id": 319,
          "name": "eth1/53/1",
          "controllingPort": 319,
          "pins": [
            {
              "a": {
                "chip": "BC58",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/53",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC58",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/53",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC58",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/53",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC58",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/53",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                320
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "320": {
        "mapping": {
          "id": 320,
          "name": "eth1/53/2",
          "controllingPort": 319,
          "pins": [
            {
              "a": {
                "chip": "BC58",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/53",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC58",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/53",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC58",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/53",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC58",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/53",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/53",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "321": {
        "mapping": {
          "id": 321,
          "name": "eth1/58/1",
          "controllingPort": 321,
          "pins": [
            {
              "a": {
                "chip": "BC59",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC59",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC59",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC59",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                322
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "322": {
        "mapping": {
          "id": 322,
          "name": "eth1/58/2",
          "controllingPort": 321,
          "pins": [
            {
              "a": {
                "chip": "BC59",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC59",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC59",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC59",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC59",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "330": {
        "mapping": {
          "id": 330,
          "name": "eth1/37/1",
          "controllingPort": 330,
          "pins": [
            {
              "a": {
                "chip": "BC60",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/37",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC60",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/37",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC60",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/37",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC60",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/37",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                331
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "331": {
        "mapping": {
          "id": 331,
          "name": "eth1/37/2",
          "controllingPort": 330,
          "pins": [
            {
              "a": {
                "chip": "BC60",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/37",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC60",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/37",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC60",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/37",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC60",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/37",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/37",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "332": {
        "mapping": {
          "id": 332,
          "name": "eth1/42/1",
          "controllingPort": 332,
          "pins": [
            {
              "a": {
                "chip": "BC61",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC61",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC61",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC61",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                333
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "333": {
        "mapping": {
          "id": 333,
          "name": "eth1/42/2",
          "controllingPort": 332,
          "pins": [
            {
              "a": {
                "chip": "BC61",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC61",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC61",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC61",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC61",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "341": {
        "mapping": {
          "id": 341,
          "name": "eth1/57/1",
          "controllingPort": 341,
          "pins": [
            {
              "a": {
                "chip": "BC62",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/57",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC62",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/57",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC62",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/57",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC62",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/57",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                342
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "342": {
        "mapping": {
          "id": 342,
          "name": "eth1/57/2",
          "controllingPort": 341,
          "pins": [
            {
              "a": {
                "chip": "BC62",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/57",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC62",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/57",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC62",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/57",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC62",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/57",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/57",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "343": {
        "mapping": {
          "id": 343,
          "name": "eth1/54/1",
          "controllingPort": 343,
          "pins": [
            {
              "a": {
                "chip": "BC63",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 0
                }
              }
            },
            {
              "a": {
                "chip": "BC63",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 1
                }
              }
            },
            {
              "a": {
                "chip": "BC63",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 2
                }
              }
            },
            {
              "a": {
                "chip": "BC63",
                "lane": 3
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 3
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 3
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 3
                    }
                  }
                ]
              }
          },
          "39": {
              "subsumedPorts": [
                344
              ],
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 0
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 1
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 2
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 3
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "344": {
        "mapping": {
          "id": 344,
          "name": "eth1/54/2",
          "controllingPort": 343,
          "pins": [
            {
              "a": {
                "chip": "BC63",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 4
                }
              }
            },
            {
              "a": {
                "chip": "BC63",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 5
                }
              }
            },
            {
              "a": {
                "chip": "BC63",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 6
                }
              }
            },
            {
              "a": {
                "chip": "BC63",
                "lane": 7
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 7
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 7
                    }
                  }
                ]
              }
          },
          "38": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "BC63",
                      "lane": 7
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 4
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 5
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 6
                    }
                  },
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    }
  },
  "chips": [
    {
      "name": "BC1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "BC4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "BC8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "BC12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "BC16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "BC20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "BC24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "BC28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "BC21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "BC17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "BC29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "BC25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "BC5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "BC0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "BC13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "BC9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "BC6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "BC2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "BC14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "BC10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "BC22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "BC18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "BC31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "BC26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "BC19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "BC23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "BC27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "BC30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "BC3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "BC7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "BC11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "BC15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "BC44",
      "type": 1,
      "physicalID": 44
    },
    {
      "name": "BC40",
      "type": 1,
      "physicalID": 40
    },
    {
      "name": "BC36",
      "type": 1,
      "physicalID": 36
    },
    {
      "name": "BC33",
      "type": 1,
      "physicalID": 33
    },
    {
      "name": "BC60",
      "type": 1,
      "physicalID": 60
    },
    {
      "name": "BC56",
      "type": 1,
      "physicalID": 56
    },
    {
      "name": "BC52",
      "type": 1,
      "physicalID": 52
    },
    {
      "name": "BC48",
      "type": 1,
      "physicalID": 48
    },
    {
      "name": "BC57",
      "type": 1,
      "physicalID": 57
    },
    {
      "name": "BC61",
      "type": 1,
      "physicalID": 61
    },
    {
      "name": "BC49",
      "type": 1,
      "physicalID": 49
    },
    {
      "name": "BC53",
      "type": 1,
      "physicalID": 53
    },
    {
      "name": "BC41",
      "type": 1,
      "physicalID": 41
    },
    {
      "name": "BC45",
      "type": 1,
      "physicalID": 45
    },
    {
      "name": "BC32",
      "type": 1,
      "physicalID": 32
    },
    {
      "name": "BC37",
      "type": 1,
      "physicalID": 37
    },
    {
      "name": "BC42",
      "type": 1,
      "physicalID": 42
    },
    {
      "name": "BC46",
      "type": 1,
      "physicalID": 46
    },
    {
      "name": "BC34",
      "type": 1,
      "physicalID": 34
    },
    {
      "name": "BC38",
      "type": 1,
      "physicalID": 38
    },
    {
      "name": "BC58",
      "type": 1,
      "physicalID": 58
    },
    {
      "name": "BC63",
      "type": 1,
      "physicalID": 63
    },
    {
      "name": "BC50",
      "type": 1,
      "physicalID": 50
    },
    {
      "name": "BC54",
      "type": 1,
      "physicalID": 54
    },
    {
      "name": "BC62",
      "type": 1,
      "physicalID": 62
    },
    {
      "name": "BC59",
      "type": 1,
      "physicalID": 59
    },
    {
      "name": "BC55",
      "type": 1,
      "physicalID": 55
    },
    {
      "name": "BC51",
      "type": 1,
      "physicalID": 51
    },
    {
      "name": "BC47",
      "type": 1,
      "physicalID": 47
    },
    {
      "name": "BC43",
      "type": 1,
      "physicalID": 43
    },
    {
      "name": "BC39",
      "type": 1,
      "physicalID": 39
    },
    {
      "name": "BC35",
      "type": 1,
      "physicalID": 35
    },
    {
      "name": "eth1/1",
      "type": 3,
      "physicalID": 0
    },
    {
      "name": "eth1/2",
      "type": 3,
      "physicalID": 1
    },
    {
      "name": "eth1/3",
      "type": 3,
      "physicalID": 2
    },
    {
      "name": "eth1/4",
      "type": 3,
      "physicalID": 3
    },
    {
      "name": "eth1/5",
      "type": 3,
      "physicalID": 4
    },
    {
      "name": "eth1/6",
      "type": 3,
      "physicalID": 5
    },
    {
      "name": "eth1/7",
      "type": 3,
      "physicalID": 6
    },
    {
      "name": "eth1/8",
      "type": 3,
      "physicalID": 7
    },
    {
      "name": "eth1/9",
      "type": 3,
      "physicalID": 8
    },
    {
      "name": "eth1/10",
      "type": 3,
      "physicalID": 9
    },
    {
      "name": "eth1/11",
      "type": 3,
      "physicalID": 10
    },
    {
      "name": "eth1/12",
      "type": 3,
      "physicalID": 11
    },
    {
      "name": "eth1/13",
      "type": 3,
      "physicalID": 12
    },
    {
      "name": "eth1/14",
      "type": 3,
      "physicalID": 13
    },
    {
      "name": "eth1/15",
      "type": 3,
      "physicalID": 14
    },
    {
      "name": "eth1/16",
      "type": 3,
      "physicalID": 15
    },
    {
      "name": "eth1/17",
      "type": 3,
      "physicalID": 16
    },
    {
      "name": "eth1/18",
      "type": 3,
      "physicalID": 17
    },
    {
      "name": "eth1/19",
      "type": 3,
      "physicalID": 18
    },
    {
      "name": "eth1/20",
      "type": 3,
      "physicalID": 19
    },
    {
      "name": "eth1/21",
      "type": 3,
      "physicalID": 20
    },
    {
      "name": "eth1/22",
      "type": 3,
      "physicalID": 21
    },
    {
      "name": "eth1/23",
      "type": 3,
      "physicalID": 22
    },
    {
      "name": "eth1/24",
      "type": 3,
      "physicalID": 23
    },
    {
      "name": "eth1/25",
      "type": 3,
      "physicalID": 24
    },
    {
      "name": "eth1/26",
      "type": 3,
      "physicalID": 25
    },
    {
      "name": "eth1/27",
      "type": 3,
      "physicalID": 26
    },
    {
      "name": "eth1/28",
      "type": 3,
      "physicalID": 27
    },
    {
      "name": "eth1/29",
      "type": 3,
      "physicalID": 28
    },
    {
      "name": "eth1/30",
      "type": 3,
      "physicalID": 29
    },
    {
      "name": "eth1/31",
      "type": 3,
      "physicalID": 30
    },
    {
      "name": "eth1/32",
      "type": 3,
      "physicalID": 31
    },
    {
      "name": "eth1/33",
      "type": 3,
      "physicalID": 32
    },
    {
      "name": "eth1/34",
      "type": 3,
      "physicalID": 33
    },
    {
      "name": "eth1/35",
      "type": 3,
      "physicalID": 34
    },
    {
      "name": "eth1/36",
      "type": 3,
      "physicalID": 35
    },
    {
      "name": "eth1/37",
      "type": 3,
      "physicalID": 36
    },
    {
      "name": "eth1/38",
      "type": 3,
      "physicalID": 37
    },
    {
      "name": "eth1/39",
      "type": 3,
      "physicalID": 38
    },
    {
      "name": "eth1/40",
      "type": 3,
      "physicalID": 39
    },
    {
      "name": "eth1/41",
      "type": 3,
      "physicalID": 40
    },
    {
      "name": "eth1/42",
      "type": 3,
      "physicalID": 41
    },
    {
      "name": "eth1/43",
      "type": 3,
      "physicalID": 42
    },
    {
      "name": "eth1/44",
      "type": 3,
      "physicalID": 43
    },
    {
      "name": "eth1/45",
      "type": 3,
      "physicalID": 44
    },
    {
      "name": "eth1/46",
      "type": 3,
      "physicalID": 45
    },
    {
      "name": "eth1/47",
      "type": 3,
      "physicalID": 46
    },
    {
      "name": "eth1/48",
      "type": 3,
      "physicalID": 47
    },
    {
      "name": "eth1/49",
      "type": 3,
      "physicalID": 48
    },
    {
      "name": "eth1/50",
      "type": 3,
      "physicalID": 49
    },
    {
      "name": "eth1/51",
      "type": 3,
      "physicalID": 50
    },
    {
      "name": "eth1/52",
      "type": 3,
      "physicalID": 51
    },
    {
      "name": "eth1/53",
      "type": 3,
      "physicalID": 52
    },
    {
      "name": "eth1/54",
      "type": 3,
      "physicalID": 53
    },
    {
      "name": "eth1/55",
      "type": 3,
      "physicalID": 54
    },
    {
      "name": "eth1/56",
      "type": 3,
      "physicalID": 55
    },
    {
      "name": "eth1/57",
      "type": 3,
      "physicalID": 56
    },
    {
      "name": "eth1/58",
      "type": 3,
      "physicalID": 57
    },
    {
      "name": "eth1/59",
      "type": 3,
      "physicalID": 58
    },
    {
      "name": "eth1/60",
      "type": 3,
      "physicalID": 59
    },
    {
      "name": "eth1/61",
      "type": 3,
      "physicalID": 60
    },
    {
      "name": "eth1/62",
      "type": 3,
      "physicalID": 61
    },
    {
      "name": "eth1/63",
      "type": 3,
      "physicalID": 62
    },
    {
      "name": "eth1/64",
      "type": 3,
      "physicalID": 63
    }
  ],
  "portConfigOverrides": [
    {
      "factor": {
        "profiles": [
          25
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 1
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 2
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 3
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          }
        ]
      }
    },
    {
      "factor": {
        "profiles": [
          38
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 1
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 2
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 3
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          }
        ]
      }
    },
    {
      "factor": {
        "profiles": [
          39
        ]
      },
      "pins": {
        "iphy": [
          {
            "id": {
              "chip": "ALL",
              "lane": 0
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 1
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 2
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 3
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 4
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 5
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 6
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 7
            },
            "tx": {
              "pre": -24,
              "pre2": 0,
              "main": 132,
              "post": -12,
              "post2": 0,
              "post3": 0
            }
          }
        ]
      }
    }
  ],
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 25
      },
      "profile": {
        "speed": 200000,
        "iphy": {
          "numLanes": 4,
          "modulation": 2,
          "fec": 11,
          "medium": 3,
          "interfaceMode": 3,
          "interfaceType": 3
        }
      }
    },
    {
      "factor": {
        "profileID": 38
      },
      "profile": {
        "speed": 400000,
        "iphy": {
          "numLanes": 4,
          "modulation": 2,
          "fec": 11,
          "medium": 3,
          "interfaceMode": 4,
          "interfaceType": 4
        }
      }
    },
    {
      "factor": {
        "profileID": 39
      },
      "profile": {
        "speed": 800000,
        "iphy": {
          "numLanes": 8,
          "modulation": 2,
          "fec": 11,
          "medium": 3,
          "interfaceMode": 4,
          "interfaceType": 4
        }
      }
    }
  ]
}
)";
} // namespace

namespace facebook {
namespace fboss {
MontblancPlatformMapping::MontblancPlatformMapping()
    : PlatformMapping(kJsonPlatformMappingStr) {}

MontblancPlatformMapping::MontblancPlatformMapping(
    const std::string& platformMappingStr)
    : PlatformMapping(platformMappingStr) {}

} // namespace fboss
} // namespace facebook
