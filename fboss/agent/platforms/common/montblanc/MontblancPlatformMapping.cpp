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
          "name": "eth1/2/1",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                2,
                3,
                4
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
              "subsumedPorts": [
                2,
                3,
                4
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
                2,
                3,
                4,
                5,
                6,
                7,
                8
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 0
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
          "name": "eth1/2/2",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 1
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
          "name": "eth1/2/3",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 2
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
          "name": "eth1/2/4",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "BC0",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/2",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "5": {
        "mapping": {
          "id": 5,
          "name": "eth1/2/5",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                6,
                7,
                8
              ],
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
              "subsumedPorts": [
                6,
                7,
                8
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "6": {
        "mapping": {
          "id": 6,
          "name": "eth1/2/6",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "7": {
        "mapping": {
          "id": 7,
          "name": "eth1/2/7",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "BC0",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/2",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC0",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/2",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "8": {
        "mapping": {
          "id": 8,
          "name": "eth1/2/8",
          "controllingPort": 1,
          "pins": [
            {
              "a": {
                "chip": "BC0",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/2",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "9": {
        "mapping": {
          "id": 9,
          "name": "eth1/1/1",
          "controllingPort": 9,
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
                10
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
    "10": {
        "mapping": {
          "id": 10,
          "name": "eth1/1/5",
          "controllingPort": 9,
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
          "name": "eth1/3/1",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                12,
                13,
                14
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
              "subsumedPorts": [
                12,
                13,
                14
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
                12,
                13,
                14,
                15,
                16,
                17,
                18
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 0
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
          "name": "eth1/3/2",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 1
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
          "name": "eth1/3/3",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 2
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
          "name": "eth1/3/4",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "BC2",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/3",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "15": {
        "mapping": {
          "id": 15,
          "name": "eth1/3/5",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                16,
                17,
                18
              ],
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
              "subsumedPorts": [
                16,
                17,
                18
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "16": {
        "mapping": {
          "id": 16,
          "name": "eth1/3/6",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "17": {
        "mapping": {
          "id": 17,
          "name": "eth1/3/7",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "BC2",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/3",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC2",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/3",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "18": {
        "mapping": {
          "id": 18,
          "name": "eth1/3/8",
          "controllingPort": 11,
          "pins": [
            {
              "a": {
                "chip": "BC2",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/3",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "19": {
        "mapping": {
          "id": 19,
          "name": "eth1/4/1",
          "controllingPort": 19,
          "pins": [
            {
              "a": {
                "chip": "BC3",
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
                "chip": "BC3",
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
                "chip": "BC3",
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
                "chip": "BC3",
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
                20
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
    "20": {
        "mapping": {
          "id": 20,
          "name": "eth1/4/5",
          "controllingPort": 19,
          "pins": [
            {
              "a": {
                "chip": "BC3",
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
                "chip": "BC3",
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
                "chip": "BC3",
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
                "chip": "BC3",
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
    "22": {
        "mapping": {
          "id": 22,
          "name": "eth1/6/1",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                23,
                24,
                25
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
              "subsumedPorts": [
                23,
                24,
                25
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
                23,
                24,
                25,
                26,
                27,
                28,
                29
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 0
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
          "name": "eth1/6/2",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 1
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
          "name": "eth1/6/3",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 2
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
          "name": "eth1/6/4",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "BC4",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/6",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "26": {
        "mapping": {
          "id": 26,
          "name": "eth1/6/5",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                27,
                28,
                29
              ],
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
              "subsumedPorts": [
                27,
                28,
                29
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "27": {
        "mapping": {
          "id": 27,
          "name": "eth1/6/6",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "28": {
        "mapping": {
          "id": 28,
          "name": "eth1/6/7",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "BC4",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/6",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC4",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/6",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "29": {
        "mapping": {
          "id": 29,
          "name": "eth1/6/8",
          "controllingPort": 22,
          "pins": [
            {
              "a": {
                "chip": "BC4",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/6",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "30": {
        "mapping": {
          "id": 30,
          "name": "eth1/5/1",
          "controllingPort": 30,
          "pins": [
            {
              "a": {
                "chip": "BC5",
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
                "chip": "BC5",
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
                "chip": "BC5",
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
                "chip": "BC5",
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
                31
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
    "31": {
        "mapping": {
          "id": 31,
          "name": "eth1/5/5",
          "controllingPort": 30,
          "pins": [
            {
              "a": {
                "chip": "BC5",
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
                "chip": "BC5",
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
                "chip": "BC5",
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
                "chip": "BC5",
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
    "33": {
        "mapping": {
          "id": 33,
          "name": "eth1/7/1",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                34,
                35,
                36
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
              "subsumedPorts": [
                34,
                35,
                36
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
                34,
                35,
                36,
                37,
                38,
                39,
                40
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 0
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
          "name": "eth1/7/2",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 1
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
          "name": "eth1/7/3",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 2
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
          "name": "eth1/7/4",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "BC6",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/7",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "37": {
        "mapping": {
          "id": 37,
          "name": "eth1/7/5",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                38,
                39,
                40
              ],
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
              "subsumedPorts": [
                38,
                39,
                40
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "38": {
        "mapping": {
          "id": 38,
          "name": "eth1/7/6",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "39": {
        "mapping": {
          "id": 39,
          "name": "eth1/7/7",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "BC6",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/7",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC6",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/7",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "40": {
        "mapping": {
          "id": 40,
          "name": "eth1/7/8",
          "controllingPort": 33,
          "pins": [
            {
              "a": {
                "chip": "BC6",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/7",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "41": {
        "mapping": {
          "id": 41,
          "name": "eth1/8/1",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "BC7",
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
                "chip": "BC7",
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
                "chip": "BC7",
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
                "chip": "BC7",
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
                42
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
    "42": {
        "mapping": {
          "id": 42,
          "name": "eth1/8/5",
          "controllingPort": 41,
          "pins": [
            {
              "a": {
                "chip": "BC7",
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
                "chip": "BC7",
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
                "chip": "BC7",
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
                "chip": "BC7",
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
    "44": {
        "mapping": {
          "id": 44,
          "name": "eth1/10/1",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                45,
                46,
                47
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
              "subsumedPorts": [
                45,
                46,
                47
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
                45,
                46,
                47,
                48,
                49,
                50,
                51
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 0
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
          "name": "eth1/10/2",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 1
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
          "name": "eth1/10/3",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 2
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
          "name": "eth1/10/4",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "BC8",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/10",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "48": {
        "mapping": {
          "id": 48,
          "name": "eth1/10/5",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                49,
                50,
                51
              ],
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
              "subsumedPorts": [
                49,
                50,
                51
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "49": {
        "mapping": {
          "id": 49,
          "name": "eth1/10/6",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "50": {
        "mapping": {
          "id": 50,
          "name": "eth1/10/7",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "BC8",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/10",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC8",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/10",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "51": {
        "mapping": {
          "id": 51,
          "name": "eth1/10/8",
          "controllingPort": 44,
          "pins": [
            {
              "a": {
                "chip": "BC8",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/10",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "52": {
        "mapping": {
          "id": 52,
          "name": "eth1/9/1",
          "controllingPort": 52,
          "pins": [
            {
              "a": {
                "chip": "BC9",
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
                "chip": "BC9",
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
                "chip": "BC9",
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
                "chip": "BC9",
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
                53
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
    "53": {
        "mapping": {
          "id": 53,
          "name": "eth1/9/5",
          "controllingPort": 52,
          "pins": [
            {
              "a": {
                "chip": "BC9",
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
                "chip": "BC9",
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
                "chip": "BC9",
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
                "chip": "BC9",
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
    "55": {
        "mapping": {
          "id": 55,
          "name": "eth1/11/1",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                56,
                57,
                58
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
              "subsumedPorts": [
                56,
                57,
                58
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
                56,
                57,
                58,
                59,
                60,
                61,
                62
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 0
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
          "name": "eth1/11/2",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 1
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
          "name": "eth1/11/3",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 2
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
          "name": "eth1/11/4",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "BC10",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/11",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "59": {
        "mapping": {
          "id": 59,
          "name": "eth1/11/5",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                60,
                61,
                62
              ],
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
              "subsumedPorts": [
                60,
                61,
                62
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "60": {
        "mapping": {
          "id": 60,
          "name": "eth1/11/6",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "61": {
        "mapping": {
          "id": 61,
          "name": "eth1/11/7",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "BC10",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/11",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC10",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/11",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "62": {
        "mapping": {
          "id": 62,
          "name": "eth1/11/8",
          "controllingPort": 55,
          "pins": [
            {
              "a": {
                "chip": "BC10",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/11",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "63": {
        "mapping": {
          "id": 63,
          "name": "eth1/12/1",
          "controllingPort": 63,
          "pins": [
            {
              "a": {
                "chip": "BC11",
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
                "chip": "BC11",
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
                "chip": "BC11",
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
                "chip": "BC11",
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
                64
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
    "64": {
        "mapping": {
          "id": 64,
          "name": "eth1/12/5",
          "controllingPort": 63,
          "pins": [
            {
              "a": {
                "chip": "BC11",
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
                "chip": "BC11",
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
                "chip": "BC11",
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
                "chip": "BC11",
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
    "66": {
        "mapping": {
          "id": 66,
          "name": "eth1/14/1",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                67,
                68,
                69
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
              "subsumedPorts": [
                67,
                68,
                69
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
                67,
                68,
                69,
                70,
                71,
                72,
                73
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 0
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
          "name": "eth1/14/2",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 1
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
          "name": "eth1/14/3",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 2
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
          "name": "eth1/14/4",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "BC12",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/14",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "70": {
        "mapping": {
          "id": 70,
          "name": "eth1/14/5",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                71,
                72,
                73
              ],
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
              "subsumedPorts": [
                71,
                72,
                73
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "71": {
        "mapping": {
          "id": 71,
          "name": "eth1/14/6",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "72": {
        "mapping": {
          "id": 72,
          "name": "eth1/14/7",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "BC12",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/14",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC12",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/14",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "73": {
        "mapping": {
          "id": 73,
          "name": "eth1/14/8",
          "controllingPort": 66,
          "pins": [
            {
              "a": {
                "chip": "BC12",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/14",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "74": {
        "mapping": {
          "id": 74,
          "name": "eth1/13/1",
          "controllingPort": 74,
          "pins": [
            {
              "a": {
                "chip": "BC13",
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
                "chip": "BC13",
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
                "chip": "BC13",
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
                "chip": "BC13",
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
                75
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
    "75": {
        "mapping": {
          "id": 75,
          "name": "eth1/13/5",
          "controllingPort": 74,
          "pins": [
            {
              "a": {
                "chip": "BC13",
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
                "chip": "BC13",
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
                "chip": "BC13",
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
                "chip": "BC13",
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
    "77": {
        "mapping": {
          "id": 77,
          "name": "eth1/15/1",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                78,
                79,
                80
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
              "subsumedPorts": [
                78,
                79,
                80
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
                78,
                79,
                80,
                81,
                82,
                83,
                84
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 0
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
          "name": "eth1/15/2",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 1
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
          "name": "eth1/15/3",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 2
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
          "name": "eth1/15/4",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "BC14",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/15",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "81": {
        "mapping": {
          "id": 81,
          "name": "eth1/15/5",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                82,
                83,
                84
              ],
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
              "subsumedPorts": [
                82,
                83,
                84
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "82": {
        "mapping": {
          "id": 82,
          "name": "eth1/15/6",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "83": {
        "mapping": {
          "id": 83,
          "name": "eth1/15/7",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "BC14",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/15",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC14",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/15",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "84": {
        "mapping": {
          "id": 84,
          "name": "eth1/15/8",
          "controllingPort": 77,
          "pins": [
            {
              "a": {
                "chip": "BC14",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/15",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "85": {
        "mapping": {
          "id": 85,
          "name": "eth1/16/1",
          "controllingPort": 85,
          "pins": [
            {
              "a": {
                "chip": "BC15",
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
                "chip": "BC15",
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
                "chip": "BC15",
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
                "chip": "BC15",
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
                86
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
    "86": {
        "mapping": {
          "id": 86,
          "name": "eth1/16/5",
          "controllingPort": 85,
          "pins": [
            {
              "a": {
                "chip": "BC15",
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
                "chip": "BC15",
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
                "chip": "BC15",
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
                "chip": "BC15",
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
    "88": {
        "mapping": {
          "id": 88,
          "name": "eth1/18/1",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                89,
                90,
                91
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
              "subsumedPorts": [
                89,
                90,
                91
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
                89,
                90,
                91,
                92,
                93,
                94,
                95
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 0
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
          "name": "eth1/18/2",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 1
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
          "name": "eth1/18/3",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 2
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
          "name": "eth1/18/4",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "BC16",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/18",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "92": {
        "mapping": {
          "id": 92,
          "name": "eth1/18/5",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                93,
                94,
                95
              ],
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
              "subsumedPorts": [
                93,
                94,
                95
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "93": {
        "mapping": {
          "id": 93,
          "name": "eth1/18/6",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "94": {
        "mapping": {
          "id": 94,
          "name": "eth1/18/7",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "BC16",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/18",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC16",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/18",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "95": {
        "mapping": {
          "id": 95,
          "name": "eth1/18/8",
          "controllingPort": 88,
          "pins": [
            {
              "a": {
                "chip": "BC16",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/18",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "96": {
        "mapping": {
          "id": 96,
          "name": "eth1/17/1",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "BC17",
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
                "chip": "BC17",
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
                "chip": "BC17",
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
                "chip": "BC17",
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
                97
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
    "97": {
        "mapping": {
          "id": 97,
          "name": "eth1/17/5",
          "controllingPort": 96,
          "pins": [
            {
              "a": {
                "chip": "BC17",
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
                "chip": "BC17",
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
                "chip": "BC17",
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
                "chip": "BC17",
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
    "99": {
        "mapping": {
          "id": 99,
          "name": "eth1/19/1",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                100,
                101,
                102
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
              "subsumedPorts": [
                100,
                101,
                102
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
                100,
                101,
                102,
                103,
                104,
                105,
                106
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 0
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
          "name": "eth1/19/2",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 1
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
          "name": "eth1/19/3",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 2
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
          "name": "eth1/19/4",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "BC18",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/19",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "103": {
        "mapping": {
          "id": 103,
          "name": "eth1/19/5",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                104,
                105,
                106
              ],
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
              "subsumedPorts": [
                104,
                105,
                106
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "104": {
        "mapping": {
          "id": 104,
          "name": "eth1/19/6",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "105": {
        "mapping": {
          "id": 105,
          "name": "eth1/19/7",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "BC18",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/19",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC18",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/19",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "106": {
        "mapping": {
          "id": 106,
          "name": "eth1/19/8",
          "controllingPort": 99,
          "pins": [
            {
              "a": {
                "chip": "BC18",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/19",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "107": {
        "mapping": {
          "id": 107,
          "name": "eth1/20/1",
          "controllingPort": 107,
          "pins": [
            {
              "a": {
                "chip": "BC19",
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
                "chip": "BC19",
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
                "chip": "BC19",
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
                "chip": "BC19",
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
                108
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
    "108": {
        "mapping": {
          "id": 108,
          "name": "eth1/20/5",
          "controllingPort": 107,
          "pins": [
            {
              "a": {
                "chip": "BC19",
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
                "chip": "BC19",
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
                "chip": "BC19",
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
                "chip": "BC19",
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
    "110": {
        "mapping": {
          "id": 110,
          "name": "eth1/22/1",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                111,
                112,
                113
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
              "subsumedPorts": [
                111,
                112,
                113
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
                111,
                112,
                113,
                114,
                115,
                116,
                117
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 0
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
          "name": "eth1/22/2",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 1
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
          "name": "eth1/22/3",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 2
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
          "name": "eth1/22/4",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "BC20",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/22",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "114": {
        "mapping": {
          "id": 114,
          "name": "eth1/22/5",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                115,
                116,
                117
              ],
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
              "subsumedPorts": [
                115,
                116,
                117
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "115": {
        "mapping": {
          "id": 115,
          "name": "eth1/22/6",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "116": {
        "mapping": {
          "id": 116,
          "name": "eth1/22/7",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "BC20",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/22",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC20",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/22",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "117": {
        "mapping": {
          "id": 117,
          "name": "eth1/22/8",
          "controllingPort": 110,
          "pins": [
            {
              "a": {
                "chip": "BC20",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/22",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "118": {
        "mapping": {
          "id": 118,
          "name": "eth1/21/1",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "BC21",
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
                "chip": "BC21",
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
                "chip": "BC21",
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
                "chip": "BC21",
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
                119
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
    "119": {
        "mapping": {
          "id": 119,
          "name": "eth1/21/5",
          "controllingPort": 118,
          "pins": [
            {
              "a": {
                "chip": "BC21",
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
                "chip": "BC21",
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
                "chip": "BC21",
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
                "chip": "BC21",
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
    "121": {
        "mapping": {
          "id": 121,
          "name": "eth1/23/1",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                122,
                123,
                124
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
              "subsumedPorts": [
                122,
                123,
                124
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
                122,
                123,
                124,
                125,
                126,
                127,
                128
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 0
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
          "name": "eth1/23/2",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 1
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
          "name": "eth1/23/3",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 2
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
          "name": "eth1/23/4",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "BC22",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/23",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "125": {
        "mapping": {
          "id": 125,
          "name": "eth1/23/5",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                126,
                127,
                128
              ],
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
              "subsumedPorts": [
                126,
                127,
                128
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "126": {
        "mapping": {
          "id": 126,
          "name": "eth1/23/6",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "127": {
        "mapping": {
          "id": 127,
          "name": "eth1/23/7",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "BC22",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/23",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC22",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/23",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "128": {
        "mapping": {
          "id": 128,
          "name": "eth1/23/8",
          "controllingPort": 121,
          "pins": [
            {
              "a": {
                "chip": "BC22",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/23",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "129": {
        "mapping": {
          "id": 129,
          "name": "eth1/24/1",
          "controllingPort": 129,
          "pins": [
            {
              "a": {
                "chip": "BC23",
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
                "chip": "BC23",
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
                "chip": "BC23",
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
                "chip": "BC23",
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
                130
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
    "130": {
        "mapping": {
          "id": 130,
          "name": "eth1/24/5",
          "controllingPort": 129,
          "pins": [
            {
              "a": {
                "chip": "BC23",
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
                "chip": "BC23",
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
                "chip": "BC23",
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
                "chip": "BC23",
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
    "132": {
        "mapping": {
          "id": 132,
          "name": "eth1/26/1",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                133,
                134,
                135
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
              "subsumedPorts": [
                133,
                134,
                135
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
                133,
                134,
                135,
                136,
                137,
                138,
                139
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 0
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
          "name": "eth1/26/2",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 1
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
          "name": "eth1/26/3",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 2
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
          "name": "eth1/26/4",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "BC24",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/26",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "136": {
        "mapping": {
          "id": 136,
          "name": "eth1/26/5",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                137,
                138,
                139
              ],
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
              "subsumedPorts": [
                137,
                138,
                139
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "137": {
        "mapping": {
          "id": 137,
          "name": "eth1/26/6",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "138": {
        "mapping": {
          "id": 138,
          "name": "eth1/26/7",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "BC24",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/26",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC24",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/26",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "139": {
        "mapping": {
          "id": 139,
          "name": "eth1/26/8",
          "controllingPort": 132,
          "pins": [
            {
              "a": {
                "chip": "BC24",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/26",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "140": {
        "mapping": {
          "id": 140,
          "name": "eth1/25/1",
          "controllingPort": 140,
          "pins": [
            {
              "a": {
                "chip": "BC25",
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
                "chip": "BC25",
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
                "chip": "BC25",
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
                "chip": "BC25",
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
                141
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
    "141": {
        "mapping": {
          "id": 141,
          "name": "eth1/25/5",
          "controllingPort": 140,
          "pins": [
            {
              "a": {
                "chip": "BC25",
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
                "chip": "BC25",
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
                "chip": "BC25",
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
                "chip": "BC25",
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
    "143": {
        "mapping": {
          "id": 143,
          "name": "eth1/27/1",
          "controllingPort": 143,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                144,
                145,
                146
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
              "subsumedPorts": [
                144,
                145,
                146
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
                144,
                145,
                146,
                147,
                148,
                149,
                150
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 0
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
          "name": "eth1/27/2",
          "controllingPort": 143,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 1
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
          "name": "eth1/27/3",
          "controllingPort": 143,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 2
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
          "name": "eth1/27/4",
          "controllingPort": 143,
          "pins": [
            {
              "a": {
                "chip": "BC26",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/27",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "147": {
        "mapping": {
          "id": 147,
          "name": "eth1/27/5",
          "controllingPort": 143,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                148,
                149,
                150
              ],
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
              "subsumedPorts": [
                148,
                149,
                150
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "148": {
        "mapping": {
          "id": 148,
          "name": "eth1/27/6",
          "controllingPort": 143,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "149": {
        "mapping": {
          "id": 149,
          "name": "eth1/27/7",
          "controllingPort": 143,
          "pins": [
            {
              "a": {
                "chip": "BC26",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/27",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC26",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/27",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "150": {
        "mapping": {
          "id": 150,
          "name": "eth1/27/8",
          "controllingPort": 143,
          "pins": [
            {
              "a": {
                "chip": "BC26",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/27",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "151": {
        "mapping": {
          "id": 151,
          "name": "eth1/28/1",
          "controllingPort": 151,
          "pins": [
            {
              "a": {
                "chip": "BC27",
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
                "chip": "BC27",
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
                "chip": "BC27",
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
                "chip": "BC27",
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
                152
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
    "152": {
        "mapping": {
          "id": 152,
          "name": "eth1/28/5",
          "controllingPort": 151,
          "pins": [
            {
              "a": {
                "chip": "BC27",
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
                "chip": "BC27",
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
                "chip": "BC27",
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
                "chip": "BC27",
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
    "154": {
        "mapping": {
          "id": 154,
          "name": "eth1/30/1",
          "controllingPort": 154,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                155,
                156,
                157
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
              "subsumedPorts": [
                155,
                156,
                157
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
                155,
                156,
                157,
                158,
                159,
                160,
                161
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 0
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
          "name": "eth1/30/2",
          "controllingPort": 154,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 1
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
          "name": "eth1/30/3",
          "controllingPort": 154,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 2
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
          "name": "eth1/30/4",
          "controllingPort": 154,
          "pins": [
            {
              "a": {
                "chip": "BC28",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/30",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "158": {
        "mapping": {
          "id": 158,
          "name": "eth1/30/5",
          "controllingPort": 154,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                159,
                160,
                161
              ],
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
              "subsumedPorts": [
                159,
                160,
                161
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "159": {
        "mapping": {
          "id": 159,
          "name": "eth1/30/6",
          "controllingPort": 154,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "160": {
        "mapping": {
          "id": 160,
          "name": "eth1/30/7",
          "controllingPort": 154,
          "pins": [
            {
              "a": {
                "chip": "BC28",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/30",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC28",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/30",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "161": {
        "mapping": {
          "id": 161,
          "name": "eth1/30/8",
          "controllingPort": 154,
          "pins": [
            {
              "a": {
                "chip": "BC28",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/30",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "162": {
        "mapping": {
          "id": 162,
          "name": "eth1/29/1",
          "controllingPort": 162,
          "pins": [
            {
              "a": {
                "chip": "BC29",
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
                "chip": "BC29",
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
                "chip": "BC29",
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
                "chip": "BC29",
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
                163
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
    "163": {
        "mapping": {
          "id": 163,
          "name": "eth1/29/5",
          "controllingPort": 162,
          "pins": [
            {
              "a": {
                "chip": "BC29",
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
                "chip": "BC29",
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
                "chip": "BC29",
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
                "chip": "BC29",
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
    "165": {
        "mapping": {
          "id": 165,
          "name": "eth1/31/1",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                166,
                167,
                168
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
              "subsumedPorts": [
                166,
                167,
                168
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
                166,
                167,
                168,
                169,
                170,
                171,
                172
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 0
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
          "name": "eth1/31/2",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 1
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
          "name": "eth1/31/3",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 2
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
          "name": "eth1/31/4",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "BC30",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/31",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "169": {
        "mapping": {
          "id": 169,
          "name": "eth1/31/5",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                170,
                171,
                172
              ],
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
              "subsumedPorts": [
                170,
                171,
                172
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "170": {
        "mapping": {
          "id": 170,
          "name": "eth1/31/6",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "171": {
        "mapping": {
          "id": 171,
          "name": "eth1/31/7",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "BC30",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/31",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC30",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/31",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "172": {
        "mapping": {
          "id": 172,
          "name": "eth1/31/8",
          "controllingPort": 165,
          "pins": [
            {
              "a": {
                "chip": "BC30",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/31",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "173": {
        "mapping": {
          "id": 173,
          "name": "eth1/32/1",
          "controllingPort": 173,
          "pins": [
            {
              "a": {
                "chip": "BC31",
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
                "chip": "BC31",
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
                "chip": "BC31",
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
                "chip": "BC31",
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
                174
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
    "174": {
        "mapping": {
          "id": 174,
          "name": "eth1/32/5",
          "controllingPort": 173,
          "pins": [
            {
              "a": {
                "chip": "BC31",
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
                "chip": "BC31",
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
                "chip": "BC31",
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
                "chip": "BC31",
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
    "176": {
        "mapping": {
          "id": 176,
          "name": "eth1/34/1",
          "controllingPort": 176,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                177,
                178,
                179
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
              "subsumedPorts": [
                177,
                178,
                179
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
                177,
                178,
                179,
                180,
                181,
                182,
                183
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 0
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
          "name": "eth1/34/2",
          "controllingPort": 176,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 1
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
          "name": "eth1/34/3",
          "controllingPort": 176,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 2
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
          "name": "eth1/34/4",
          "controllingPort": 176,
          "pins": [
            {
              "a": {
                "chip": "BC32",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/34",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "180": {
        "mapping": {
          "id": 180,
          "name": "eth1/34/5",
          "controllingPort": 176,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                181,
                182,
                183
              ],
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
              "subsumedPorts": [
                181,
                182,
                183
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "181": {
        "mapping": {
          "id": 181,
          "name": "eth1/34/6",
          "controllingPort": 176,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "182": {
        "mapping": {
          "id": 182,
          "name": "eth1/34/7",
          "controllingPort": 176,
          "pins": [
            {
              "a": {
                "chip": "BC32",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/34",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC32",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/34",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "183": {
        "mapping": {
          "id": 183,
          "name": "eth1/34/8",
          "controllingPort": 176,
          "pins": [
            {
              "a": {
                "chip": "BC32",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/34",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "184": {
        "mapping": {
          "id": 184,
          "name": "eth1/33/1",
          "controllingPort": 184,
          "pins": [
            {
              "a": {
                "chip": "BC33",
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
                "chip": "BC33",
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
                "chip": "BC33",
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
                "chip": "BC33",
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
                185
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
    "185": {
        "mapping": {
          "id": 185,
          "name": "eth1/33/5",
          "controllingPort": 184,
          "pins": [
            {
              "a": {
                "chip": "BC33",
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
                "chip": "BC33",
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
                "chip": "BC33",
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
                "chip": "BC33",
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
    "187": {
        "mapping": {
          "id": 187,
          "name": "eth1/35/1",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                188,
                189,
                190
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
              "subsumedPorts": [
                188,
                189,
                190
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
                188,
                189,
                190,
                191,
                192,
                193,
                194
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 0
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
          "name": "eth1/35/2",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 1
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
          "name": "eth1/35/3",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 2
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
          "name": "eth1/35/4",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "BC34",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/35",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "191": {
        "mapping": {
          "id": 191,
          "name": "eth1/35/5",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                192,
                193,
                194
              ],
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
              "subsumedPorts": [
                192,
                193,
                194
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "192": {
        "mapping": {
          "id": 192,
          "name": "eth1/35/6",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "193": {
        "mapping": {
          "id": 193,
          "name": "eth1/35/7",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "BC34",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/35",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC34",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/35",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "194": {
        "mapping": {
          "id": 194,
          "name": "eth1/35/8",
          "controllingPort": 187,
          "pins": [
            {
              "a": {
                "chip": "BC34",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/35",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "195": {
        "mapping": {
          "id": 195,
          "name": "eth1/36/1",
          "controllingPort": 195,
          "pins": [
            {
              "a": {
                "chip": "BC35",
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
                "chip": "BC35",
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
                "chip": "BC35",
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
                "chip": "BC35",
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
                196
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
    "196": {
        "mapping": {
          "id": 196,
          "name": "eth1/36/5",
          "controllingPort": 195,
          "pins": [
            {
              "a": {
                "chip": "BC35",
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
                "chip": "BC35",
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
                "chip": "BC35",
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
                "chip": "BC35",
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
    "198": {
        "mapping": {
          "id": 198,
          "name": "eth1/38/1",
          "controllingPort": 198,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                199,
                200,
                201
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
              "subsumedPorts": [
                199,
                200,
                201
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
                199,
                200,
                201,
                202,
                203,
                204,
                205
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 0
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
          "name": "eth1/38/2",
          "controllingPort": 198,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 1
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
          "name": "eth1/38/3",
          "controllingPort": 198,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 2
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
          "name": "eth1/38/4",
          "controllingPort": 198,
          "pins": [
            {
              "a": {
                "chip": "BC36",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/38",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "202": {
        "mapping": {
          "id": 202,
          "name": "eth1/38/5",
          "controllingPort": 198,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                203,
                204,
                205
              ],
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
              "subsumedPorts": [
                203,
                204,
                205
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "203": {
        "mapping": {
          "id": 203,
          "name": "eth1/38/6",
          "controllingPort": 198,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "204": {
        "mapping": {
          "id": 204,
          "name": "eth1/38/7",
          "controllingPort": 198,
          "pins": [
            {
              "a": {
                "chip": "BC36",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/38",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC36",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/38",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "205": {
        "mapping": {
          "id": 205,
          "name": "eth1/38/8",
          "controllingPort": 198,
          "pins": [
            {
              "a": {
                "chip": "BC36",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/38",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "206": {
        "mapping": {
          "id": 206,
          "name": "eth1/37/1",
          "controllingPort": 206,
          "pins": [
            {
              "a": {
                "chip": "BC37",
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
                "chip": "BC37",
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
                "chip": "BC37",
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
                "chip": "BC37",
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
                207
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
    "207": {
        "mapping": {
          "id": 207,
          "name": "eth1/37/5",
          "controllingPort": 206,
          "pins": [
            {
              "a": {
                "chip": "BC37",
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
                "chip": "BC37",
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
                "chip": "BC37",
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
                "chip": "BC37",
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
    "209": {
        "mapping": {
          "id": 209,
          "name": "eth1/39/1",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                210,
                211,
                212
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
              "subsumedPorts": [
                210,
                211,
                212
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
                210,
                211,
                212,
                213,
                214,
                215,
                216
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 0
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
          "name": "eth1/39/2",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 1
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
          "name": "eth1/39/3",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 2
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
          "name": "eth1/39/4",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "BC38",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/39",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "213": {
        "mapping": {
          "id": 213,
          "name": "eth1/39/5",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                214,
                215,
                216
              ],
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
              "subsumedPorts": [
                214,
                215,
                216
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "214": {
        "mapping": {
          "id": 214,
          "name": "eth1/39/6",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "215": {
        "mapping": {
          "id": 215,
          "name": "eth1/39/7",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "BC38",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/39",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC38",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/39",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "216": {
        "mapping": {
          "id": 216,
          "name": "eth1/39/8",
          "controllingPort": 209,
          "pins": [
            {
              "a": {
                "chip": "BC38",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/39",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "217": {
        "mapping": {
          "id": 217,
          "name": "eth1/40/1",
          "controllingPort": 217,
          "pins": [
            {
              "a": {
                "chip": "BC39",
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
                "chip": "BC39",
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
                "chip": "BC39",
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
                "chip": "BC39",
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
                218
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
    "218": {
        "mapping": {
          "id": 218,
          "name": "eth1/40/5",
          "controllingPort": 217,
          "pins": [
            {
              "a": {
                "chip": "BC39",
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
                "chip": "BC39",
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
                "chip": "BC39",
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
                "chip": "BC39",
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
    "220": {
        "mapping": {
          "id": 220,
          "name": "eth1/42/1",
          "controllingPort": 220,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                221,
                222,
                223
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
              "subsumedPorts": [
                221,
                222,
                223
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
                221,
                222,
                223,
                224,
                225,
                226,
                227
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 0
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
          "name": "eth1/42/2",
          "controllingPort": 220,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 1
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
          "name": "eth1/42/3",
          "controllingPort": 220,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 2
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
          "name": "eth1/42/4",
          "controllingPort": 220,
          "pins": [
            {
              "a": {
                "chip": "BC40",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/42",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "224": {
        "mapping": {
          "id": 224,
          "name": "eth1/42/5",
          "controllingPort": 220,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                225,
                226,
                227
              ],
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
              "subsumedPorts": [
                225,
                226,
                227
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "225": {
        "mapping": {
          "id": 225,
          "name": "eth1/42/6",
          "controllingPort": 220,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "226": {
        "mapping": {
          "id": 226,
          "name": "eth1/42/7",
          "controllingPort": 220,
          "pins": [
            {
              "a": {
                "chip": "BC40",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/42",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC40",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/42",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "227": {
        "mapping": {
          "id": 227,
          "name": "eth1/42/8",
          "controllingPort": 220,
          "pins": [
            {
              "a": {
                "chip": "BC40",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/42",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "228": {
        "mapping": {
          "id": 228,
          "name": "eth1/41/1",
          "controllingPort": 228,
          "pins": [
            {
              "a": {
                "chip": "BC41",
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
                "chip": "BC41",
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
                "chip": "BC41",
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
                "chip": "BC41",
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
                229
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
    "229": {
        "mapping": {
          "id": 229,
          "name": "eth1/41/5",
          "controllingPort": 228,
          "pins": [
            {
              "a": {
                "chip": "BC41",
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
                "chip": "BC41",
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
                "chip": "BC41",
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
                "chip": "BC41",
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
    "231": {
        "mapping": {
          "id": 231,
          "name": "eth1/43/1",
          "controllingPort": 231,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                232,
                233,
                234
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
              "subsumedPorts": [
                232,
                233,
                234
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
                232,
                233,
                234,
                235,
                236,
                237,
                238
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 0
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
          "name": "eth1/43/2",
          "controllingPort": 231,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 1
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
          "name": "eth1/43/3",
          "controllingPort": 231,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 2
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
          "name": "eth1/43/4",
          "controllingPort": 231,
          "pins": [
            {
              "a": {
                "chip": "BC42",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/43",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "235": {
        "mapping": {
          "id": 235,
          "name": "eth1/43/5",
          "controllingPort": 231,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                236,
                237,
                238
              ],
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
              "subsumedPorts": [
                236,
                237,
                238
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "236": {
        "mapping": {
          "id": 236,
          "name": "eth1/43/6",
          "controllingPort": 231,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "237": {
        "mapping": {
          "id": 237,
          "name": "eth1/43/7",
          "controllingPort": 231,
          "pins": [
            {
              "a": {
                "chip": "BC42",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/43",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC42",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/43",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "238": {
        "mapping": {
          "id": 238,
          "name": "eth1/43/8",
          "controllingPort": 231,
          "pins": [
            {
              "a": {
                "chip": "BC42",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/43",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "239": {
        "mapping": {
          "id": 239,
          "name": "eth1/44/1",
          "controllingPort": 239,
          "pins": [
            {
              "a": {
                "chip": "BC43",
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
                "chip": "BC43",
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
                "chip": "BC43",
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
                "chip": "BC43",
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
                240
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
    "240": {
        "mapping": {
          "id": 240,
          "name": "eth1/44/5",
          "controllingPort": 239,
          "pins": [
            {
              "a": {
                "chip": "BC43",
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
                "chip": "BC43",
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
                "chip": "BC43",
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
                "chip": "BC43",
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
    "242": {
        "mapping": {
          "id": 242,
          "name": "eth1/46/1",
          "controllingPort": 242,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                243,
                244,
                245
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
              "subsumedPorts": [
                243,
                244,
                245
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
                243,
                244,
                245,
                246,
                247,
                248,
                249
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 0
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
          "name": "eth1/46/2",
          "controllingPort": 242,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 1
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
          "name": "eth1/46/3",
          "controllingPort": 242,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 2
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
          "name": "eth1/46/4",
          "controllingPort": 242,
          "pins": [
            {
              "a": {
                "chip": "BC44",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/46",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "246": {
        "mapping": {
          "id": 246,
          "name": "eth1/46/5",
          "controllingPort": 242,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                247,
                248,
                249
              ],
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
              "subsumedPorts": [
                247,
                248,
                249
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "247": {
        "mapping": {
          "id": 247,
          "name": "eth1/46/6",
          "controllingPort": 242,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "248": {
        "mapping": {
          "id": 248,
          "name": "eth1/46/7",
          "controllingPort": 242,
          "pins": [
            {
              "a": {
                "chip": "BC44",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/46",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC44",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/46",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "249": {
        "mapping": {
          "id": 249,
          "name": "eth1/46/8",
          "controllingPort": 242,
          "pins": [
            {
              "a": {
                "chip": "BC44",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/46",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "250": {
        "mapping": {
          "id": 250,
          "name": "eth1/45/1",
          "controllingPort": 250,
          "pins": [
            {
              "a": {
                "chip": "BC45",
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
                "chip": "BC45",
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
                "chip": "BC45",
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
                "chip": "BC45",
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
                251
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
    "251": {
        "mapping": {
          "id": 251,
          "name": "eth1/45/5",
          "controllingPort": 250,
          "pins": [
            {
              "a": {
                "chip": "BC45",
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
                "chip": "BC45",
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
                "chip": "BC45",
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
                "chip": "BC45",
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
    "253": {
        "mapping": {
          "id": 253,
          "name": "eth1/47/1",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                254,
                255,
                256
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
              "subsumedPorts": [
                254,
                255,
                256
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
                254,
                255,
                256,
                257,
                258,
                259,
                260
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 0
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
          "name": "eth1/47/2",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 1
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
          "name": "eth1/47/3",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 2
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
          "name": "eth1/47/4",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "BC46",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/47",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "257": {
        "mapping": {
          "id": 257,
          "name": "eth1/47/5",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                258,
                259,
                260
              ],
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
              "subsumedPorts": [
                258,
                259,
                260
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "258": {
        "mapping": {
          "id": 258,
          "name": "eth1/47/6",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "259": {
        "mapping": {
          "id": 259,
          "name": "eth1/47/7",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "BC46",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/47",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC46",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/47",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "260": {
        "mapping": {
          "id": 260,
          "name": "eth1/47/8",
          "controllingPort": 253,
          "pins": [
            {
              "a": {
                "chip": "BC46",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/47",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "261": {
        "mapping": {
          "id": 261,
          "name": "eth1/48/1",
          "controllingPort": 261,
          "pins": [
            {
              "a": {
                "chip": "BC47",
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
                "chip": "BC47",
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
                "chip": "BC47",
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
                "chip": "BC47",
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
                262
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
    "262": {
        "mapping": {
          "id": 262,
          "name": "eth1/48/5",
          "controllingPort": 261,
          "pins": [
            {
              "a": {
                "chip": "BC47",
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
                "chip": "BC47",
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
                "chip": "BC47",
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
                "chip": "BC47",
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
    "264": {
        "mapping": {
          "id": 264,
          "name": "eth1/50/1",
          "controllingPort": 264,
          "pins": [
            {
              "a": {
                "chip": "BC48",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                265,
                266,
                267
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
              "subsumedPorts": [
                265,
                266,
                267
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
                265,
                266,
                267,
                268,
                269,
                270,
                271
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 0
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
          "name": "eth1/50/2",
          "controllingPort": 264,
          "pins": [
            {
              "a": {
                "chip": "BC48",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 1
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
          "name": "eth1/50/3",
          "controllingPort": 264,
          "pins": [
            {
              "a": {
                "chip": "BC48",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 2
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
          "name": "eth1/50/4",
          "controllingPort": 264,
          "pins": [
            {
              "a": {
                "chip": "BC48",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/50",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "268": {
        "mapping": {
          "id": 268,
          "name": "eth1/50/5",
          "controllingPort": 264,
          "pins": [
            {
              "a": {
                "chip": "BC48",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                269,
                270,
                271
              ],
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
              "subsumedPorts": [
                269,
                270,
                271
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "269": {
        "mapping": {
          "id": 269,
          "name": "eth1/50/6",
          "controllingPort": 264,
          "pins": [
            {
              "a": {
                "chip": "BC48",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "270": {
        "mapping": {
          "id": 270,
          "name": "eth1/50/7",
          "controllingPort": 264,
          "pins": [
            {
              "a": {
                "chip": "BC48",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/50",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC48",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/50",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "271": {
        "mapping": {
          "id": 271,
          "name": "eth1/50/8",
          "controllingPort": 264,
          "pins": [
            {
              "a": {
                "chip": "BC48",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/50",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "272": {
        "mapping": {
          "id": 272,
          "name": "eth1/49/1",
          "controllingPort": 272,
          "pins": [
            {
              "a": {
                "chip": "BC49",
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
                "chip": "BC49",
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
                "chip": "BC49",
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
                "chip": "BC49",
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
                273
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
    "273": {
        "mapping": {
          "id": 273,
          "name": "eth1/49/5",
          "controllingPort": 272,
          "pins": [
            {
              "a": {
                "chip": "BC49",
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
                "chip": "BC49",
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
                "chip": "BC49",
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
                "chip": "BC49",
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
    "275": {
        "mapping": {
          "id": 275,
          "name": "eth1/51/1",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "BC50",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                276,
                277,
                278
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
              "subsumedPorts": [
                276,
                277,
                278
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
                276,
                277,
                278,
                279,
                280,
                281,
                282
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 0
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
          "name": "eth1/51/2",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "BC50",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 1
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
          "name": "eth1/51/3",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "BC50",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 2
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
          "name": "eth1/51/4",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "BC50",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/51",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "279": {
        "mapping": {
          "id": 279,
          "name": "eth1/51/5",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "BC50",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                280,
                281,
                282
              ],
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
              "subsumedPorts": [
                280,
                281,
                282
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "280": {
        "mapping": {
          "id": 280,
          "name": "eth1/51/6",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "BC50",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "281": {
        "mapping": {
          "id": 281,
          "name": "eth1/51/7",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "BC50",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/51",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC50",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/51",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "282": {
        "mapping": {
          "id": 282,
          "name": "eth1/51/8",
          "controllingPort": 275,
          "pins": [
            {
              "a": {
                "chip": "BC50",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/51",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "283": {
        "mapping": {
          "id": 283,
          "name": "eth1/52/1",
          "controllingPort": 283,
          "pins": [
            {
              "a": {
                "chip": "BC51",
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
                "chip": "BC51",
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
                "chip": "BC51",
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
                "chip": "BC51",
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
                284
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
    "284": {
        "mapping": {
          "id": 284,
          "name": "eth1/52/5",
          "controllingPort": 283,
          "pins": [
            {
              "a": {
                "chip": "BC51",
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
                "chip": "BC51",
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
                "chip": "BC51",
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
                "chip": "BC51",
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
    "286": {
        "mapping": {
          "id": 286,
          "name": "eth1/54/1",
          "controllingPort": 286,
          "pins": [
            {
              "a": {
                "chip": "BC52",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                287,
                288,
                289
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
              "subsumedPorts": [
                287,
                288,
                289
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
                287,
                288,
                289,
                290,
                291,
                292,
                293
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 0
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
          "name": "eth1/54/2",
          "controllingPort": 286,
          "pins": [
            {
              "a": {
                "chip": "BC52",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 1
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
          "name": "eth1/54/3",
          "controllingPort": 286,
          "pins": [
            {
              "a": {
                "chip": "BC52",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 2
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
          "name": "eth1/54/4",
          "controllingPort": 286,
          "pins": [
            {
              "a": {
                "chip": "BC52",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/54",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "290": {
        "mapping": {
          "id": 290,
          "name": "eth1/54/5",
          "controllingPort": 286,
          "pins": [
            {
              "a": {
                "chip": "BC52",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                291,
                292,
                293
              ],
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
              "subsumedPorts": [
                291,
                292,
                293
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "291": {
        "mapping": {
          "id": 291,
          "name": "eth1/54/6",
          "controllingPort": 286,
          "pins": [
            {
              "a": {
                "chip": "BC52",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "292": {
        "mapping": {
          "id": 292,
          "name": "eth1/54/7",
          "controllingPort": 286,
          "pins": [
            {
              "a": {
                "chip": "BC52",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/54",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC52",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/54",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "293": {
        "mapping": {
          "id": 293,
          "name": "eth1/54/8",
          "controllingPort": 286,
          "pins": [
            {
              "a": {
                "chip": "BC52",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/54",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "294": {
        "mapping": {
          "id": 294,
          "name": "eth1/53/1",
          "controllingPort": 294,
          "pins": [
            {
              "a": {
                "chip": "BC53",
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
                "chip": "BC53",
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
                "chip": "BC53",
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
                "chip": "BC53",
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
                295
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
    "295": {
        "mapping": {
          "id": 295,
          "name": "eth1/53/5",
          "controllingPort": 294,
          "pins": [
            {
              "a": {
                "chip": "BC53",
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
                "chip": "BC53",
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
                "chip": "BC53",
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
                "chip": "BC53",
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
    "297": {
        "mapping": {
          "id": 297,
          "name": "eth1/55/1",
          "controllingPort": 297,
          "pins": [
            {
              "a": {
                "chip": "BC54",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                298,
                299,
                300
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
              "subsumedPorts": [
                298,
                299,
                300
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
                298,
                299,
                300,
                301,
                302,
                303,
                304
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 0
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
          "name": "eth1/55/2",
          "controllingPort": 297,
          "pins": [
            {
              "a": {
                "chip": "BC54",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 1
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
          "name": "eth1/55/3",
          "controllingPort": 297,
          "pins": [
            {
              "a": {
                "chip": "BC54",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 2
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
          "name": "eth1/55/4",
          "controllingPort": 297,
          "pins": [
            {
              "a": {
                "chip": "BC54",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/55",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "301": {
        "mapping": {
          "id": 301,
          "name": "eth1/55/5",
          "controllingPort": 297,
          "pins": [
            {
              "a": {
                "chip": "BC54",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                302,
                303,
                304
              ],
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
              "subsumedPorts": [
                302,
                303,
                304
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "302": {
        "mapping": {
          "id": 302,
          "name": "eth1/55/6",
          "controllingPort": 297,
          "pins": [
            {
              "a": {
                "chip": "BC54",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "303": {
        "mapping": {
          "id": 303,
          "name": "eth1/55/7",
          "controllingPort": 297,
          "pins": [
            {
              "a": {
                "chip": "BC54",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/55",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC54",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/55",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "304": {
        "mapping": {
          "id": 304,
          "name": "eth1/55/8",
          "controllingPort": 297,
          "pins": [
            {
              "a": {
                "chip": "BC54",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/55",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "305": {
        "mapping": {
          "id": 305,
          "name": "eth1/56/1",
          "controllingPort": 305,
          "pins": [
            {
              "a": {
                "chip": "BC55",
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
                "chip": "BC55",
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
                "chip": "BC55",
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
                "chip": "BC55",
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
                306
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
    "306": {
        "mapping": {
          "id": 306,
          "name": "eth1/56/5",
          "controllingPort": 305,
          "pins": [
            {
              "a": {
                "chip": "BC55",
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
                "chip": "BC55",
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
                "chip": "BC55",
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
                "chip": "BC55",
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
    "308": {
        "mapping": {
          "id": 308,
          "name": "eth1/58/1",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "BC56",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                309,
                310,
                311
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
              "subsumedPorts": [
                309,
                310,
                311
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
                309,
                310,
                311,
                312,
                313,
                314,
                315
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 0
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
          "name": "eth1/58/2",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "BC56",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 1
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
          "name": "eth1/58/3",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "BC56",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 2
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
          "name": "eth1/58/4",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "BC56",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/58",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "312": {
        "mapping": {
          "id": 312,
          "name": "eth1/58/5",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "BC56",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                313,
                314,
                315
              ],
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
              "subsumedPorts": [
                313,
                314,
                315
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "313": {
        "mapping": {
          "id": 313,
          "name": "eth1/58/6",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "BC56",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "314": {
        "mapping": {
          "id": 314,
          "name": "eth1/58/7",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "BC56",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/58",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC56",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/58",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "315": {
        "mapping": {
          "id": 315,
          "name": "eth1/58/8",
          "controllingPort": 308,
          "pins": [
            {
              "a": {
                "chip": "BC56",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/58",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "316": {
        "mapping": {
          "id": 316,
          "name": "eth1/57/1",
          "controllingPort": 316,
          "pins": [
            {
              "a": {
                "chip": "BC57",
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
                "chip": "BC57",
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
                "chip": "BC57",
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
                "chip": "BC57",
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
                317
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
    "317": {
        "mapping": {
          "id": 317,
          "name": "eth1/57/5",
          "controllingPort": 316,
          "pins": [
            {
              "a": {
                "chip": "BC57",
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
                "chip": "BC57",
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
                "chip": "BC57",
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
                "chip": "BC57",
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
    "319": {
        "mapping": {
          "id": 319,
          "name": "eth1/59/1",
          "controllingPort": 319,
          "pins": [
            {
              "a": {
                "chip": "BC58",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                320,
                321,
                322
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
              "subsumedPorts": [
                320,
                321,
                322
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
                320,
                321,
                322,
                323,
                324,
                325,
                326
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 0
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
          "name": "eth1/59/2",
          "controllingPort": 319,
          "pins": [
            {
              "a": {
                "chip": "BC58",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 1
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
          "name": "eth1/59/3",
          "controllingPort": 319,
          "pins": [
            {
              "a": {
                "chip": "BC58",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 2
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
          "name": "eth1/59/4",
          "controllingPort": 319,
          "pins": [
            {
              "a": {
                "chip": "BC58",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/59",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "323": {
        "mapping": {
          "id": 323,
          "name": "eth1/59/5",
          "controllingPort": 319,
          "pins": [
            {
              "a": {
                "chip": "BC58",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                324,
                325,
                326
              ],
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
              "subsumedPorts": [
                324,
                325,
                326
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "324": {
        "mapping": {
          "id": 324,
          "name": "eth1/59/6",
          "controllingPort": 319,
          "pins": [
            {
              "a": {
                "chip": "BC58",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "325": {
        "mapping": {
          "id": 325,
          "name": "eth1/59/7",
          "controllingPort": 319,
          "pins": [
            {
              "a": {
                "chip": "BC58",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/59",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC58",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/59",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "326": {
        "mapping": {
          "id": 326,
          "name": "eth1/59/8",
          "controllingPort": 319,
          "pins": [
            {
              "a": {
                "chip": "BC58",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/59",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "327": {
        "mapping": {
          "id": 327,
          "name": "eth1/60/1",
          "controllingPort": 327,
          "pins": [
            {
              "a": {
                "chip": "BC59",
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
                "chip": "BC59",
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
                "chip": "BC59",
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
                "chip": "BC59",
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
                328
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
    "328": {
        "mapping": {
          "id": 328,
          "name": "eth1/60/5",
          "controllingPort": 327,
          "pins": [
            {
              "a": {
                "chip": "BC59",
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
                "chip": "BC59",
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
                "chip": "BC59",
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
                "chip": "BC59",
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
    "330": {
        "mapping": {
          "id": 330,
          "name": "eth1/62/1",
          "controllingPort": 330,
          "pins": [
            {
              "a": {
                "chip": "BC60",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                331,
                332,
                333
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
              "subsumedPorts": [
                331,
                332,
                333
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
                331,
                332,
                333,
                334,
                335,
                336,
                337
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 0
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
          "name": "eth1/62/2",
          "controllingPort": 330,
          "pins": [
            {
              "a": {
                "chip": "BC60",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 1
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
          "name": "eth1/62/3",
          "controllingPort": 330,
          "pins": [
            {
              "a": {
                "chip": "BC60",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 2
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
          "name": "eth1/62/4",
          "controllingPort": 330,
          "pins": [
            {
              "a": {
                "chip": "BC60",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/62",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "334": {
        "mapping": {
          "id": 334,
          "name": "eth1/62/5",
          "controllingPort": 330,
          "pins": [
            {
              "a": {
                "chip": "BC60",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                335,
                336,
                337
              ],
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
              "subsumedPorts": [
                335,
                336,
                337
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "335": {
        "mapping": {
          "id": 335,
          "name": "eth1/62/6",
          "controllingPort": 330,
          "pins": [
            {
              "a": {
                "chip": "BC60",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "336": {
        "mapping": {
          "id": 336,
          "name": "eth1/62/7",
          "controllingPort": 330,
          "pins": [
            {
              "a": {
                "chip": "BC60",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/62",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC60",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/62",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "337": {
        "mapping": {
          "id": 337,
          "name": "eth1/62/8",
          "controllingPort": 330,
          "pins": [
            {
              "a": {
                "chip": "BC60",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/62",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "338": {
        "mapping": {
          "id": 338,
          "name": "eth1/61/1",
          "controllingPort": 338,
          "pins": [
            {
              "a": {
                "chip": "BC61",
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
                "chip": "BC61",
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
                "chip": "BC61",
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
                "chip": "BC61",
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
                339
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
    "339": {
        "mapping": {
          "id": 339,
          "name": "eth1/61/5",
          "controllingPort": 338,
          "pins": [
            {
              "a": {
                "chip": "BC61",
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
                "chip": "BC61",
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
                "chip": "BC61",
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
                "chip": "BC61",
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
    "341": {
        "mapping": {
          "id": 341,
          "name": "eth1/63/1",
          "controllingPort": 341,
          "pins": [
            {
              "a": {
                "chip": "BC62",
                "lane": 0
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 0
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                342,
                343,
                344
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
              "subsumedPorts": [
                342,
                343,
                344
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
                342,
                343,
                344,
                345,
                346,
                347,
                348
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
          },
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 0
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 0
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
          "name": "eth1/63/2",
          "controllingPort": 341,
          "pins": [
            {
              "a": {
                "chip": "BC62",
                "lane": 1
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 1
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 1
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 1
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
          "name": "eth1/63/3",
          "controllingPort": 341,
          "pins": [
            {
              "a": {
                "chip": "BC62",
                "lane": 2
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 2
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 2
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 2
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
          "name": "eth1/63/4",
          "controllingPort": 341,
          "pins": [
            {
              "a": {
                "chip": "BC62",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/63",
                      "lane": 3
                    }
                  }
                ]
              }
          }
        }
    },
    "345": {
        "mapping": {
          "id": 345,
          "name": "eth1/63/5",
          "controllingPort": 341,
          "pins": [
            {
              "a": {
                "chip": "BC62",
                "lane": 4
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 4
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "25": {
              "subsumedPorts": [
                346,
                347,
                348
              ],
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
              "subsumedPorts": [
                346,
                347,
                348
              ],
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
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 4
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 4
                    }
                  }
                ]
              }
          }
        }
    },
    "346": {
        "mapping": {
          "id": 346,
          "name": "eth1/63/6",
          "controllingPort": 341,
          "pins": [
            {
              "a": {
                "chip": "BC62",
                "lane": 5
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 5
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 5
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 5
                    }
                  }
                ]
              }
          }
        }
    },
    "347": {
        "mapping": {
          "id": 347,
          "name": "eth1/63/7",
          "controllingPort": 341,
          "pins": [
            {
              "a": {
                "chip": "BC62",
                "lane": 6
              },
              "z": {
                "end": {
                  "chip": "eth1/63",
                  "lane": 6
                }
              }
            }
          ],
          "portType": 0
        },
        "supportedProfiles": {
          "42": {
              "pins": {
                "iphy": [
                  {
                    "id": {
                      "chip": "BC62",
                      "lane": 6
                    }
                  }
                ],
                "transceiver": [
                  {
                    "id": {
                      "chip": "eth1/63",
                      "lane": 6
                    }
                  }
                ]
              }
          }
        }
    },
    "348": {
        "mapping": {
          "id": 348,
          "name": "eth1/63/8",
          "controllingPort": 341,
          "pins": [
            {
              "a": {
                "chip": "BC62",
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
          "42": {
              "pins": {
                "iphy": [
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
                      "chip": "eth1/63",
                      "lane": 7
                    }
                  }
                ]
              }
          }
        }
    },
    "349": {
        "mapping": {
          "id": 349,
          "name": "eth1/64/1",
          "controllingPort": 349,
          "pins": [
            {
              "a": {
                "chip": "BC63",
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
                "chip": "BC63",
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
                "chip": "BC63",
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
                "chip": "BC63",
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
                350
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
    "350": {
        "mapping": {
          "id": 350,
          "name": "eth1/64/5",
          "controllingPort": 349,
          "pins": [
            {
              "a": {
                "chip": "BC63",
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
                "chip": "BC63",
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
                "chip": "BC63",
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
                "chip": "BC63",
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
    }
  },
  "chips": [
    {
      "name": "BC1",
      "type": 1,
      "physicalID": 1
    },
    {
      "name": "BC0",
      "type": 1,
      "physicalID": 0
    },
    {
      "name": "BC2",
      "type": 1,
      "physicalID": 2
    },
    {
      "name": "BC3",
      "type": 1,
      "physicalID": 3
    },
    {
      "name": "BC5",
      "type": 1,
      "physicalID": 5
    },
    {
      "name": "BC4",
      "type": 1,
      "physicalID": 4
    },
    {
      "name": "BC6",
      "type": 1,
      "physicalID": 6
    },
    {
      "name": "BC7",
      "type": 1,
      "physicalID": 7
    },
    {
      "name": "BC9",
      "type": 1,
      "physicalID": 9
    },
    {
      "name": "BC8",
      "type": 1,
      "physicalID": 8
    },
    {
      "name": "BC10",
      "type": 1,
      "physicalID": 10
    },
    {
      "name": "BC11",
      "type": 1,
      "physicalID": 11
    },
    {
      "name": "BC13",
      "type": 1,
      "physicalID": 13
    },
    {
      "name": "BC12",
      "type": 1,
      "physicalID": 12
    },
    {
      "name": "BC14",
      "type": 1,
      "physicalID": 14
    },
    {
      "name": "BC15",
      "type": 1,
      "physicalID": 15
    },
    {
      "name": "BC17",
      "type": 1,
      "physicalID": 17
    },
    {
      "name": "BC16",
      "type": 1,
      "physicalID": 16
    },
    {
      "name": "BC18",
      "type": 1,
      "physicalID": 18
    },
    {
      "name": "BC19",
      "type": 1,
      "physicalID": 19
    },
    {
      "name": "BC21",
      "type": 1,
      "physicalID": 21
    },
    {
      "name": "BC20",
      "type": 1,
      "physicalID": 20
    },
    {
      "name": "BC22",
      "type": 1,
      "physicalID": 22
    },
    {
      "name": "BC23",
      "type": 1,
      "physicalID": 23
    },
    {
      "name": "BC25",
      "type": 1,
      "physicalID": 25
    },
    {
      "name": "BC24",
      "type": 1,
      "physicalID": 24
    },
    {
      "name": "BC26",
      "type": 1,
      "physicalID": 26
    },
    {
      "name": "BC27",
      "type": 1,
      "physicalID": 27
    },
    {
      "name": "BC29",
      "type": 1,
      "physicalID": 29
    },
    {
      "name": "BC28",
      "type": 1,
      "physicalID": 28
    },
    {
      "name": "BC30",
      "type": 1,
      "physicalID": 30
    },
    {
      "name": "BC31",
      "type": 1,
      "physicalID": 31
    },
    {
      "name": "BC33",
      "type": 1,
      "physicalID": 33
    },
    {
      "name": "BC32",
      "type": 1,
      "physicalID": 32
    },
    {
      "name": "BC34",
      "type": 1,
      "physicalID": 34
    },
    {
      "name": "BC35",
      "type": 1,
      "physicalID": 35
    },
    {
      "name": "BC37",
      "type": 1,
      "physicalID": 37
    },
    {
      "name": "BC36",
      "type": 1,
      "physicalID": 36
    },
    {
      "name": "BC38",
      "type": 1,
      "physicalID": 38
    },
    {
      "name": "BC39",
      "type": 1,
      "physicalID": 39
    },
    {
      "name": "BC41",
      "type": 1,
      "physicalID": 41
    },
    {
      "name": "BC40",
      "type": 1,
      "physicalID": 40
    },
    {
      "name": "BC42",
      "type": 1,
      "physicalID": 42
    },
    {
      "name": "BC43",
      "type": 1,
      "physicalID": 43
    },
    {
      "name": "BC45",
      "type": 1,
      "physicalID": 45
    },
    {
      "name": "BC44",
      "type": 1,
      "physicalID": 44
    },
    {
      "name": "BC46",
      "type": 1,
      "physicalID": 46
    },
    {
      "name": "BC47",
      "type": 1,
      "physicalID": 47
    },
    {
      "name": "BC49",
      "type": 1,
      "physicalID": 49
    },
    {
      "name": "BC48",
      "type": 1,
      "physicalID": 48
    },
    {
      "name": "BC50",
      "type": 1,
      "physicalID": 50
    },
    {
      "name": "BC51",
      "type": 1,
      "physicalID": 51
    },
    {
      "name": "BC53",
      "type": 1,
      "physicalID": 53
    },
    {
      "name": "BC52",
      "type": 1,
      "physicalID": 52
    },
    {
      "name": "BC54",
      "type": 1,
      "physicalID": 54
    },
    {
      "name": "BC55",
      "type": 1,
      "physicalID": 55
    },
    {
      "name": "BC57",
      "type": 1,
      "physicalID": 57
    },
    {
      "name": "BC56",
      "type": 1,
      "physicalID": 56
    },
    {
      "name": "BC58",
      "type": 1,
      "physicalID": 58
    },
    {
      "name": "BC59",
      "type": 1,
      "physicalID": 59
    },
    {
      "name": "BC61",
      "type": 1,
      "physicalID": 61
    },
    {
      "name": "BC60",
      "type": 1,
      "physicalID": 60
    },
    {
      "name": "BC62",
      "type": 1,
      "physicalID": 62
    },
    {
      "name": "BC63",
      "type": 1,
      "physicalID": 63
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
          42
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
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          }
        ]
      }
    },
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
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 1
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 2
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 3
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
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
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 1
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 2
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 3
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
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
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 1
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 2
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 3
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 4
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 5
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 6
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          },
          {
            "id": {
              "chip": "ALL",
              "lane": 7
            },
            "tx": {
              "pre": -36,
              "pre2": 14,
              "main": 112,
              "post": 0,
              "post2": 0,
              "post3": 0,
              "pre3": -4
            }
          }
        ]
      }
    }
  ],
  "platformSupportedProfiles": [
    {
      "factor": {
        "profileID": 42
      },
      "profile": {
        "speed": 106250,
        "iphy": {
          "numLanes": 1,
          "modulation": 2,
          "fec": 544,
          "medium": 3,
          "interfaceMode": 3,
          "interfaceType": 3
        }
      }
    },
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
